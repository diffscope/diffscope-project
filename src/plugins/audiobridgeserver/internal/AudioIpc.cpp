// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "AudioIpc.h"

#include <algorithm>
#include <array>
#include <bit>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <future>
#include <limits>
#include <mutex>
#include <thread>
#include <utility>
#include <vector>

#include <QThread>
#include <QUuid>

#include <audio/ProjectAudioContext.h>
#include <iox2/iceoryx2.hpp>

namespace AudioBridge::Internal {

    namespace {

        static_assert(std::endian::native == std::endian::little);

        constexpr auto pendingRenderPollInterval = std::chrono::microseconds(150);
        constexpr auto dispatcherWaitInterval = std::chrono::milliseconds(2);
        constexpr auto waitSetErrorBackoff = std::chrono::milliseconds(1);

        QByteArray randomToken() {
            return QUuid::createUuid().toRfc4122().toHex();
        }

        constexpr auto ipc = iox2::ServiceType::Ipc;
        using Payload = iox2::bb::Slice<AudioSampleV1>;
        using Server = iox2::Server<ipc, AudioRequestV1, void, Payload, AudioReplyHeaderV1>;
        using Request = iox2::ActiveRequest<ipc, AudioRequestV1, void, Payload, AudioReplyHeaderV1>;
        using Loan = iox2::ResponseMutUninit<ipc, Payload, AudioReplyHeaderV1>;

        void sendError(Request &request, AudioReplyStatus error) {
            auto response = request.loan_slice_uninit(0);
            if (response.has_value()) {
                response.value().user_header_mut() = {request.payload(), error, 0};
                (void) send(assume_init(std::move(response.value())));
            }
        }

        struct Endpoint {
            const std::shared_ptr<AudioPort> port;
            Server server;
            iox2::Listener<ipc> listener;
            std::optional<iox2::WaitSetGuard<ipc>> attachment;
            std::optional<Request> request;
            std::optional<Loan> loan;
            quint64 lastSequence{};

            Endpoint(std::shared_ptr<AudioPort> port, Server server, iox2::Listener<ipc> listener)
                : port(std::move(port)), server(std::move(server)), listener(std::move(listener)) {
            }

            ~Endpoint() {
                port->suspend();
            }

            void pump() {
                (void) listener.try_wait([](iox2::EventActivation) {});
                if (request) {
                    const auto result = port->takeResult();
                    if (!result)
                        return;
                    const auto payload = request->payload();
                    port->complete(payload, [&] {
                        if (!request->is_connected())
                            return false;
                        if (*result == AudioReplyStatus::Ok) {
                            loan->user_header_mut() = {payload, *result, 0};
                            return send(assume_init(std::move(*loan))).has_value();
                        }
                        loan.reset();
                        sendError(*request, *result);
                        return false;
                    });
                    loan.reset();
                    request.reset();
                }

                auto received = server.receive();
                if (!received.has_value() || !received.value().has_value())
                    return;
                auto &active = received.value().value();
                if (!active.is_connected())
                    return;
                const auto payload = active.payload();
                if (payload.sequence == 0 || payload.sequence <= lastSequence) {
                    sendError(active, AudioReplyStatus::InvalidRequest);
                    return;
                }
                lastSequence = payload.sequence;
                if (const auto status = port->check(payload); status != AudioReplyStatus::Ok) {
                    sendError(active, status);
                    return;
                }
                auto response = active.loan_slice_uninit(quint64(payload.frames) * port->channels);
                if (!response.has_value()) {
                    sendError(active, AudioReplyStatus::ReadFailed);
                    return;
                }
                if (const auto status = port->enqueue(payload, response.value().payload_mut().data());
                    status != AudioReplyStatus::Ok) {
                    sendError(active, status);
                    return;
                }
                request.emplace(std::move(active));
                loan.emplace(std::move(response.value()));
            }
        };

    }

    AudioPort::AudioPort(quint32 maxFrames, int channels, std::function<void()> notifyCompletion)
        : audioName(BridgeAudioServicePrefix + randomToken()),
          wakeName(BridgeWakeServicePrefix + audioName.right(BridgeAudioPortTokenHexDigits)),
          maxFrames(maxFrames), channels(channels), m_notifyCompletion(std::move(notifyCompletion)),
          m_thread([this] {
              QThread::currentThread()->setPriority(QThread::TimeCriticalPriority);
              renderLoop();
          }) {
    }

    AudioPort::~AudioPort() {
        {
            const std::lock_guard lock(m_mutex);
            m_stopping = true;
        }
        m_changed.notify_all();
        m_thread.join();
    }

    void AudioPort::configure(Audio::ProjectAudioRenderSession *render, Mode mode, quint32 generation, qint64 position) {
        suspend();
        const std::lock_guard lock(m_mutex);
        m_render = render;
        m_mode = mode;
        m_generation = generation;
        m_position = position;
    }

    void AudioPort::suspend() {
        std::unique_lock lock(m_mutex);
        invalidateLocked();
        m_changed.wait(lock, [this] { return !m_rendering; });
    }

    std::optional<QFuture<void>> AudioPort::suspendAsynchronously() {
        const std::lock_guard lock(m_mutex);
        invalidateLocked();
        if (!m_rendering)
            return {};
        Q_ASSERT(!m_suspension);
        m_suspension.emplace();
        m_suspension->start();
        return m_suspension->future();
    }

    void AudioPort::invalidateLocked() {
        m_generation = BridgeNoAudioGeneration;
        m_render = nullptr;
        if (m_job) {
            m_job.reset();
            m_result = AudioReplyStatus::Cancelled;
            m_notifyCompletion();
        }
    }

    qint64 AudioPort::position() const {
        const std::lock_guard lock(m_mutex);
        return m_position;
    }

    AudioReplyStatus AudioPort::checkLocked(const AudioRequestV1 &request) const {
        if (request.frames == 0 || request.frames > maxFrames)
            return AudioReplyStatus::InvalidRequest;
        if (m_generation == BridgeNoAudioGeneration || m_mode == Mode::Idle || !m_render)
            return AudioReplyStatus::NotReady;
        if (request.generation != m_generation)
            return AudioReplyStatus::Cancelled;
        if ((m_mode == Mode::Preview && request.projectFrame != 0) ||
            request.projectFrame < 0 || request.projectFrame > std::numeric_limits<qint64>::max() - request.frames)
            return AudioReplyStatus::InvalidRequest;
        return AudioReplyStatus::Ok;
    }

    AudioReplyStatus AudioPort::check(const AudioRequestV1 &request) const {
        const std::lock_guard lock(m_mutex);
        return checkLocked(request);
    }

    AudioReplyStatus AudioPort::enqueue(const AudioRequestV1 &request, float *samples) {
        const std::lock_guard lock(m_mutex);
        if (const auto status = checkLocked(request); status != AudioReplyStatus::Ok)
            return status;
        m_job = Job{request.projectFrame, request.frames, samples};
        m_changed.notify_all();
        return AudioReplyStatus::Ok;
    }

    std::optional<AudioReplyStatus> AudioPort::takeResult() {
        const std::lock_guard lock(m_mutex);
        return std::exchange(m_result, std::nullopt);
    }

    void AudioPort::complete(const AudioRequestV1 &request, const std::function<bool()> &send) {
        const std::lock_guard lock(m_mutex);
        if (request.generation != m_generation || !send())
            return;
        if (m_mode != Mode::Preview)
            m_position = request.projectFrame + request.frames;
    }

    void AudioPort::renderLoop() {
        std::unique_lock lock(m_mutex);
        while (true) {
            m_changed.wait(lock, [this] { return m_stopping || m_job.has_value(); });
            if (!m_job)
                break;
            const auto job = *std::exchange(m_job, std::nullopt);
            auto *render = m_render;
            const auto mode = m_mode;
            m_rendering = true;
            lock.unlock();

            std::array<float *, BridgeMaxAudioChannels> buffers;
            for (int channel = 0; channel < channels; ++channel)
                buffers[channel] = job.samples + qsizetype(channel) * job.frames;
            const auto rendered = mode == Mode::Preview
                ? render->renderPlaybackBlockIfReady(buffers.data(), job.frames)
                : render->renderBlock(job.position, buffers.data(), job.frames);

            lock.lock();
            m_rendering = false;
            m_result = rendered == job.frames ? AudioReplyStatus::Ok
                                             : mode == Mode::Preview ? AudioReplyStatus::NotReady : AudioReplyStatus::ReadFailed;
            if (m_suspension) {
                m_suspension->finish();
                m_suspension.reset();
            }
            m_changed.notify_all();
            m_notifyCompletion();
        }
    }

    struct AudioIpc::Dispatcher {
        struct State {
            std::optional<iox2::Node<ipc>> node;
            std::optional<iox2::WaitSet<ipc>> waitSet;
            std::vector<std::unique_ptr<Endpoint>> endpoints;

            bool open(const std::shared_ptr<AudioPort> &port) {
                if (!node || !waitSet)
                    return false;
                const auto name = iox2::ServiceName::create(port->audioName.constData());
                const auto eventName = iox2::ServiceName::create(port->wakeName.constData());
                if (!name.has_value() || !eventName.has_value())
                    return false;
                auto service = node->service_builder(name.value()).request_response<AudioRequestV1, Payload>()
                    .response_user_header<AudioReplyHeaderV1>().max_clients(1).max_servers(1).max_nodes(2)
                    .max_active_requests_per_client(1).max_response_buffer_size(1)
                    .enable_safe_overflow_for_requests(false).enable_safe_overflow_for_responses(true).create();
                if (!service.has_value())
                    return false;
                auto server = service.value().server_builder()
                    .initial_max_slice_len(quint64(port->maxFrames) * port->channels)
                    .backpressure_strategy(iox2::BackpressureStrategy::DiscardData).create();
                if (!server.has_value())
                    return false;

                auto event = node->service_builder(eventName.value()).event()
                    .max_nodes(2).max_listeners(1).max_notifiers(1).create();
                if (!event.has_value())
                    return false;
                auto listener = event.value().listener_builder().create();
                if (!listener.has_value())
                    return false;
                auto endpoint = std::make_unique<Endpoint>(port, std::move(server.value()), std::move(listener.value()));
                auto attachment = waitSet->attach_notification(endpoint->listener);
                if (!attachment.has_value())
                    return false;
                endpoint->attachment.emplace(std::move(attachment.value()));
                endpoints.push_back(std::move(endpoint));
                return true;
            }
        };

        using Command = std::function<void(State &)>;
        std::mutex mutex;
        std::condition_variable changed;
        std::vector<Command> commands;
        quint64 completionSerial{};
        bool stopping{};
        std::thread thread;

        Dispatcher() : thread([this] { run(); }) {
        }

        ~Dispatcher() {
            {
                const std::lock_guard lock(mutex);
                stopping = true;
            }
            changed.notify_all();
            thread.join();
        }

        void post(Command command) {
            const std::lock_guard lock(mutex);
            commands.push_back(std::move(command));
            changed.notify_all();
        }

        void notifyCompletion() {
            const std::lock_guard lock(mutex);
            ++completionSerial;
            changed.notify_all();
        }

        void run() {
            State state;
            auto node = iox2::NodeBuilder().signal_handling_mode(iox2::SignalHandlingMode::Disabled).create<ipc>();
            auto waitSet = iox2::WaitSetBuilder().signal_handling_mode(iox2::SignalHandlingMode::Disabled).create<ipc>();
            if (node.has_value() && waitSet.has_value()) {
                state.node.emplace(std::move(node.value()));
                state.waitSet.emplace(std::move(waitSet.value()));
            }
            while (true) {
                std::vector<Command> work;
                quint64 serial;
                {
                    std::unique_lock lock(mutex);
                    if (state.endpoints.empty())
                        changed.wait(lock, [this] { return stopping || !commands.empty(); });
                    if (stopping)
                        break;
                    work.swap(commands);
                    serial = completionSerial;
                }
                for (auto &command : work)
                    command(state);
                if (state.endpoints.empty())
                    continue;

                bool pending = false;
                for (const auto &endpoint : state.endpoints) {
                    endpoint->pump();
                    pending |= endpoint->request.has_value();
                }
                if (pending) {
                    std::unique_lock lock(mutex);
                    changed.wait_for(lock, pendingRenderPollInterval, [this, serial] {
                        return stopping || completionSerial != serial || !commands.empty();
                    });
                    continue;
                }
                if (const auto result = state.waitSet->wait_and_process_once_with_timeout(
                        [](iox2::WaitSetAttachmentId<ipc>) { return iox2::CallbackProgression::Continue; },
                        iox2::bb::Duration::from_millis(dispatcherWaitInterval.count()));
                    !result.has_value())
                    std::this_thread::sleep_for(waitSetErrorBackoff);
            }
        }
    };

    AudioIpc::AudioIpc() : m_dispatcher(std::make_unique<Dispatcher>()) {
    }

    AudioIpc::~AudioIpc() = default;

    std::shared_ptr<AudioPort> AudioIpc::open(quint32 maxFrames, int channels) {
        auto port = std::make_shared<AudioPort>(maxFrames, channels, [dispatcher = m_dispatcher.get()] {
            dispatcher->notifyCompletion();
        });
        auto promise = std::make_shared<std::promise<bool>>();
        auto result = promise->get_future();
        m_dispatcher->post([port, promise](Dispatcher::State &state) { promise->set_value(state.open(port)); });
        return result.get() ? port : nullptr;
    }

    void AudioIpc::close(const std::shared_ptr<AudioPort> &port) {
        port->suspend();
        m_dispatcher->post([port](Dispatcher::State &state) {
            std::erase_if(state.endpoints, [&](const auto &endpoint) { return endpoint->port == port; });
        });
    }

}
