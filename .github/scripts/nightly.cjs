// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

const fs = require('node:fs');
const path = require('node:path');
const crypto = require('node:crypto');
const { execFileSync } = require('node:child_process');

const nightlyPattern = /^v(\d+\.\d+\.\d+-nightly\.(\d{8})\.([1-9]\d*))$/;
const attemptKind = 'diffscope-nightly-build';

function projectVersion() {
    const match = fs.readFileSync('CMakeLists.txt', 'utf8').match(/project\s*\(\s*(\w+)\s+VERSION\s+(\d+\.\d+\.\d+)/);
    if (!match) {
        throw new Error('Could not read the application name and version from CMakeLists.txt');
    }
    return { name: match[1], version: match[2] };
}

async function nightlyRefs(github, repo) {
    const { data } = await github.rest.git.listMatchingRefs({ ...repo, ref: 'tags/v' });
    return data.map(ref => {
        const name = ref.ref.slice('refs/tags/'.length);
        const match = name.match(nightlyPattern);
        if (!match) return null;
        const sequence = Number(match[3]);
        if (!Number.isSafeInteger(sequence)) throw new Error('Invalid nightly sequence in tag ' + name);
        return { ref, name, semver: match[1], date: match[2], sequence };
    }).filter(Boolean).sort((a, b) => b.date.localeCompare(a.date) || b.sequence - a.sequence);
}

async function readAttempt(github, repo, entry) {
    if (entry.ref.object.type !== 'tag') return null;
    const { data: tag } = await github.rest.git.getTag({ ...repo, tag_sha: entry.ref.object.sha });
    let metadata;
    try {
        metadata = JSON.parse(tag.message);
    } catch {
        return null;
    }
    if (metadata?.kind !== attemptKind) return null;
    if (tag.object.type !== 'commit' || metadata.source_sha !== tag.object.sha ||
        metadata.semver !== entry.semver || metadata.utc_date !== entry.date ||
        metadata.sequence !== entry.sequence || !/^[1-9]\d*$/.test(String(metadata.run_id))) {
        throw new Error('Invalid build attempt metadata in tag ' + entry.name);
    }
    return { ...metadata, tag: entry.name };
}

async function findAttempt(github, repo, refs, runId) {
    for (const entry of refs) {
        const attempt = await readAttempt(github, repo, entry);
        if (attempt && (runId === undefined || String(attempt.run_id) === String(runId))) {
            return attempt;
        }
    }
    return null;
}

async function check({ github, context, core }) {
    core.setOutput('should_build', 'false');
    const refs = await nightlyRefs(github, context.repo);
    const current = Number(process.env.GITHUB_RUN_ATTEMPT) > 1
        ? await findAttempt(github, context.repo, refs, context.runId) : null;
    if (current && current.source_sha !== context.sha) {
        throw new Error('The existing build attempt belongs to a different source commit');
    }
    if (!current) {
        const previous = await findAttempt(github, context.repo, refs);
        if (previous) {
            if (previous.source_sha === context.sha) {
                core.notice('Skipping nightly: this commit was already attempted in ' + previous.tag);
                return;
            }
            const { data: comparison } = await github.rest.repos.compareCommits({
                ...context.repo, base: previous.source_sha, head: context.sha
            });
            if (comparison.ahead_by === 0) {
                core.notice('Skipping nightly: no new commits since ' + previous.tag);
                return;
            }
        }
    }

    const runs = await github.paginate(github.rest.actions.listWorkflowRuns, {
        ...context.repo, workflow_id: 'dev-build.yml', head_sha: context.sha, per_page: 100
    });
    const matching = runs.filter(run => run.head_sha === context.sha &&
        (run.event === 'push' || run.event === 'workflow_dispatch'));
    matching.sort((a, b) => Date.parse(b.created_at) - Date.parse(a.created_at) || b.id - a.id);
    if (matching.length) {
        const { data: latest } = await github.rest.actions.getWorkflowRun({
            ...context.repo, run_id: matching[0].id
        });
        if (latest.status !== 'completed' || latest.conclusion !== 'success') {
            core.notice('Skipping nightly: latest Dev Build is ' + latest.status + '/' +
                latest.conclusion + ' (' + latest.html_url + ')');
            return;
        }
    }
    core.setOutput('should_build', 'true');
    core.notice(current ? 'Resuming ' + current.tag : 'New commits are eligible for a nightly build');
}

async function prepare({ github, context, core }) {
    const sourceSha = process.env.SOURCE_SHA;
    const buildType = process.env.BUILD_TYPE;
    if (!/^[0-9a-f]{40}$/.test(sourceSha)) throw new Error('A full source commit SHA is required');
    if (!['dev', 'nightly'].includes(buildType)) throw new Error('Unsupported shared build type');
    const project = projectVersion();
    let identifier = process.env.VERSION_IDENTIFIER || '';
    let semver;
    let tag = '';
    if (buildType === 'nightly') {
        if (context.ref !== 'refs/heads/main' || sourceSha !== context.sha ||
            context.repo.owner !== 'diffscope' || context.repo.repo !== 'diffscope-project') {
            throw new Error('Nightly versions must be reserved for the triggering main commit');
        }
        const refs = await nightlyRefs(github, context.repo);
        const current = Number(process.env.GITHUB_RUN_ATTEMPT) > 1
            ? await findAttempt(github, context.repo, refs, context.runId) : null;
        if (current) {
            if (current.source_sha !== sourceSha || !current.semver.startsWith(project.version + '-nightly.')) {
                throw new Error('The reserved nightly version does not match this source');
            }
            identifier = current.utc_date + '.' + current.sequence;
            semver = current.semver;
            tag = current.tag;
        } else {
            const startedAt = new Date().toISOString();
            const utcDate = startedAt.slice(0, 10).replaceAll('-', '');
            const sequence = Math.max(0, ...refs.filter(entry => entry.date === utcDate).map(entry => entry.sequence)) + 1;
            identifier = utcDate + '.' + sequence;
            semver = project.version + '-nightly.' + identifier;
            tag = 'v' + semver;
            const metadata = {
                kind: attemptKind, run_id: context.runId, source_sha: sourceSha,
                semver, utc_date: utcDate, sequence, started_at: startedAt
            };
            const { data: annotatedTag } = await github.rest.git.createTag({
                ...context.repo, tag, message: JSON.stringify(metadata), object: sourceSha, type: 'commit',
                tagger: {
                    name: 'github-actions[bot]',
                    email: '41898282+github-actions[bot]@users.noreply.github.com',
                    date: startedAt
                }
            });
            await github.rest.git.createRef({ ...context.repo, ref: 'refs/tags/' + tag, sha: annotatedTag.sha });
        }
    } else {
        semver = project.version + '+' + identifier;
    }
    core.setOutput('semver', semver);
    core.setOutput('tag', tag);
    core.setOutput('application_name', project.name + '_' + buildType);
    core.setOutput('version_identifier', identifier);
    await core.summary.addHeading('Build version').addTable([
        ['Version', semver], ['Source commit', sourceSha], ['Tag', tag || '(none)']
    ]).write();
}

function expectedAssets(applicationName, semver) {
    const base = applicationName + '_' + semver.replace(/[.\-+]/g, '_');
    return [
        base + '_Windows_amd64_installer.exe',
        base + '_Windows_amd64_portable.zip',
        base + '_Windows_amd64_debug_symbols.7z',
        base + '_macOS_arm64.dmg',
        base + '_macOS_arm64_debug_symbols.7z'
    ];
}

async function fileDigest(filename) {
    const hash = crypto.createHash('sha256');
    for await (const chunk of fs.createReadStream(filename)) hash.update(chunk);
    return hash.digest('hex');
}

async function assetDigest(asset) {
    if (/^sha256:[0-9a-f]{64}$/i.test(asset.digest || '')) {
        return asset.digest.slice('sha256:'.length).toLowerCase();
    }
    const response = await fetch(asset.url, {
        headers: {
            Accept: 'application/octet-stream',
            Authorization: 'Bearer ' + process.env.GH_TOKEN,
            'X-GitHub-Api-Version': '2026-03-10'
        }
    });
    if (!response.ok) throw new Error('Could not download existing release asset ' + asset.name);
    const hash = crypto.createHash('sha256');
    for await (const chunk of response.body) hash.update(chunk);
    return hash.digest('hex');
}

async function publish({ github, context, core }) {
    const tag = process.env.NIGHTLY_TAG;
    const match = tag.match(nightlyPattern);
    if (!match) throw new Error('Invalid nightly tag');
    const semver = match[1];
    const names = expectedAssets(process.env.APPLICATION_NAME, semver);
    const { data: ref } = await github.rest.git.getRef({ ...context.repo, ref: 'tags/' + tag });
    const attempt = await readAttempt(github, context.repo, {
        ref, name: tag, semver, date: match[2], sequence: Number(match[3])
    });
    if (!attempt || attempt.source_sha !== process.env.SOURCE_SHA || String(attempt.run_id) !== String(context.runId)) {
        throw new Error('The release tag does not belong to this build attempt');
    }
    let release;
    try {
        ({ data: release } = await github.rest.repos.getReleaseByTag({ ...context.repo, tag }));
    } catch (error) {
        if (error.status !== 404) throw error;
    }
    if (!release) {
        const releases = await github.paginate(github.rest.repos.listReleases, { ...context.repo, per_page: 100 });
        const previous = releases.filter(item => !item.draft && item.prerelease &&
            nightlyPattern.test(item.tag_name) && item.tag_name !== tag)
            .sort((a, b) => Date.parse(b.published_at) - Date.parse(a.published_at))[0];
        const { data: notes } = await github.rest.repos.generateReleaseNotes({
            ...context.repo, tag_name: tag, target_commitish: attempt.source_sha,
            ...(previous ? { previous_tag_name: previous.tag_name } : {})
        });
        ({ data: release } = await github.rest.repos.createRelease({
            ...context.repo, tag_name: tag, target_commitish: attempt.source_sha,
            name: semver, body: notes.body, draft: true, prerelease: true, make_latest: 'false'
        }));
    }
    if (!release.prerelease) throw new Error('The existing release is not a prerelease');

    let assets = await github.paginate(github.rest.repos.listReleaseAssets, {
        ...context.repo, release_id: release.id, per_page: 100
    });
    if (assets.some(asset => !names.includes(asset.name))) throw new Error('Unexpected files in the nightly release');
    if (release.draft) {
        for (const name of names) {
            const filename = path.join('release-assets', name);
            const stat = fs.statSync(filename);
            if (!stat.isFile() || stat.size === 0) throw new Error('Missing or empty build asset ' + name);
            const existing = assets.find(asset => asset.name === name);
            if (existing && existing.state === 'uploaded') {
                if (existing.size === stat.size && await assetDigest(existing) === await fileDigest(filename)) {
                    continue;
                }
            }
            if (existing) {
                await github.rest.repos.deleteReleaseAsset({ ...context.repo, asset_id: existing.id });
            }
            execFileSync('gh', ['release', 'upload', tag, filename, '--repo',
                context.repo.owner + '/' + context.repo.repo], { stdio: 'inherit' });
        }
        assets = await github.paginate(github.rest.repos.listReleaseAssets, {
            ...context.repo, release_id: release.id, per_page: 100
        });
    }
    if (assets.length !== names.length || names.some(name =>
        !assets.some(asset => asset.name === name && asset.state === 'uploaded' && asset.size > 0))) {
        throw new Error('Nightly release assets are incomplete');
    }
    if (release.draft) {
        ({ data: release } = await github.rest.repos.updateRelease({
            ...context.repo, release_id: release.id, draft: false, prerelease: true, make_latest: 'false'
        }));
    }
    core.setOutput('release_id', String(release.id));
    await core.summary.addLink('Published ' + semver, release.html_url).write();
}

async function dispatch({ github, core }) {
    const releaseId = process.env.RELEASE_ID;
    if (!/^[1-9]\d*$/.test(releaseId)) throw new Error('Invalid release ID');
    const owner = 'diffscope';
    const repo = 'catalogs';
    const deadline = Date.now() + 30 * 60 * 1000;
    const { data } = await github.request('POST /repos/{owner}/{repo}/actions/workflows/{workflow_id}/dispatches', {
        owner, repo, workflow_id: 'update-diffscope-nightly.yml', ref: 'main',
        inputs: { release_id: releaseId }, headers: { 'X-GitHub-Api-Version': '2026-03-10' }
    });
    if (!data.workflow_run_id) throw new Error('The dispatch API did not return a catalogs workflow run ID');
    core.notice('Catalogs update: ' + data.html_url);
    await core.summary.addLink('Catalogs update', data.html_url).write();
    while (Date.now() < deadline) {
        const { data: run } = await github.rest.actions.getWorkflowRun({
            owner, repo, run_id: data.workflow_run_id
        });
        if (run.status === 'completed') {
            if (run.conclusion !== 'success') {
                throw new Error('Catalogs update finished with ' + run.conclusion + ': ' + run.html_url);
            }
            return;
        }
        await new Promise(resolve => setTimeout(resolve, 15000));
    }
    throw new Error('Timed out waiting 30 minutes for catalogs update: ' + data.html_url);
}

module.exports = { check, prepare, publish, dispatch };
