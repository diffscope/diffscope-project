# View-Model ↔ Document Binding Prompt

Use this prompt to guide new view-model bindings for future document elements. Keep property specifics out; focus on state design, controller-driven transitions, transactions, and synchronization patterns. Derive patterns from existing implementations (e.g., `TrackViewModelContextData`, `LabelViewModelContextData`, `TempoViewModelContextData`).

## Core Goals
- Mirror document collections to view-model collections with clear ownership and mapping tables in both directions.
- Drive UI interactions through controllers; never mutate document state directly from QML.
- Guard against infinite loops by conditioning view→doc updates on active states.
- Wrap mutating operations in transactions with start/commit/abort semantics tied to state entry/exit.

- Instrument every state with entry/exit logging (use `qCInfo` with a per-context logging category) to trace flows during debugging, even for states without handlers.

## State Machine Design
- Build a `QStateMachine` with explicit states for idle, rubber-band selection, move/drag, edit/adjust flows, and per-property operations as needed.
- Keep transitions driven by signals emitted from interaction controllers and internal guards (e.g., started/not-started, commit/abort, finish).
- Example (move flow, list rotation):
  - `idle` → `movePending` on `moveTransactionWillStart` (from controller).
  - `movePending` → `moveProcessing` on `moveTransactionStarted`; → `idle` on `moveTransactionNotStarted`.
  - `moveProcessing` → `moveCommitting` on `moveTransactionWillCommit`; → `moveAborting` on `moveTransactionWillAbort`.
  - `moveCommitting`/`moveAborting` → `idle` (reset guards).
- Example (edit flow, text-like):
  - `idle` → `namePending` on `nameTransactionWillStart`.
  - `namePending` → `nameProgressing` on `nameTransactionStarted`; → `idle` if not started.
  - `nameProgressing` → `nameCommitting` on commit; → `nameAborting` on abort.
  - `nameCommitting`/`nameAborting` → `idle`.
- Rubber-band selection: `idle` → `rubberBandDragging` on start, back to `idle` on finish.

## Controller-Driven Transitions
- Wire controller signals to emit local signals that the state machine consumes (two patterns):
  1) **Started/Committed/Aborted**: drag/move and text edits use started/commit/abort triplets.
  2) **Started/Finished**: toggle/slider/height edits use started/finished pairs.
- Set the current target item/index when the controller signals a start; clear it on commit/finish/abort handlers.
- For rotations or list moves: only propagate view→doc when in the move-processing state; otherwise apply doc→view rotations.

## Transactions
- Begin a transaction when entering the corresponding "pending" state. Abort immediately if the controller could not start.
- On commit states: if the new value differs, write to the document and `commitTransaction` with a descriptive label; otherwise `abortTransaction`.
- On abort states: always `abortTransaction` and reset local guards (`target`, flags like `moveChanged`).
- Track whether any change occurred during move/drag (`moveChanged`) to avoid committing no-op transactions.

## Synchronization Patterns
- Maintain bidirectional maps: `doc -> view` and `view -> doc`. Insert/remove bindings on collection signals (`itemInserted`/`itemRemoved`), not "aboutTo" when you need the item fully constructed.
- When binding a new item:
  - Create the view-model item, insert into both maps and the view-model collection at the correct index.
  - Connect doc→view signals to update view items, guarded by equality checks.
  - Connect view→doc signals but gate them with state checks (only honor during the relevant progressing/doing states; otherwise revert the view to the doc value).
  - Initialize view properties from the doc model after wiring connections.
- Selection sync: listen to document selection model `itemSelected` and mark the view item selected; initialize selection for pre-selected items after binding.
- Rotation sync: doc→view rotations apply when *not* moving; view→doc rotations apply only while the move state is active, and should mark a change flag.

## Example Snippets
- **Doc→View guarded update** (avoid loops):
  ```cpp
  connect(control, &ControlType::propertyChanged, viewItem, [=](auto value) {
      if (viewItem->property() == value) return;
      viewItem->setProperty(value);
  });
  ```
- **View→Doc gated by state**:
  ```cpp
  connect(viewItem, &ViewType::propertyChanged, docItem, [=] {
      if (!stateMachine->configuration().contains(propertyProgressingState)) {
          viewItem->setProperty(docItem->property());
          return;
      }
      // defer actual write to commit handler
  });
  ```
- **Transaction commit handler**:
  ```cpp
  void ContextData::onNameCommittingStateEntered() {
      if (!target || nameTxId == Invalid) { target = {}; return; }
      auto viewItem = viewMap.value(target);
      if (viewItem->name() == target->name()) {
          tx->abortTransaction(nameTxId);
      } else {
          target->setName(viewItem->name());
          tx->commitTransaction(nameTxId, tr("Renaming item"));
      }
      nameTxId = {}; target = {};
  }
  ```
- **Rotate handling**:
  ```cpp
  connect(docList, &List::rotated, this, [=](int l, int m, int r) {
      if (stateMachine->configuration().contains(moveProcessingState)) return;
      viewList->rotate(l, m, r);
  });
  connect(viewList, &ViewList::rotated, this, [=](int l, int m, int r) {
      if (!stateMachine->configuration().contains(moveProcessingState)) return;
      moveChanged = true;
      docList->rotate(l, m, r);
  });
  ```

## Implementation Checklist
- Define states and transitions before binding to controllers; start the state machine immediately.
- Create controllers via context helper methods; hook all relevant signals to emit local transition signals and set the current target.
- Bind document collections first, then replay existing selection to the view.
- For each commit/finish handler: compare values, write document, commit transaction; otherwise abort. Always reset `target` and flags.
- Keep all strings ASCII; add concise comments only where non-obvious.

Use this prompt verbatim when extending bindings to new document elements to maintain consistent interaction, transaction, and synchronization behavior across the codebase.
