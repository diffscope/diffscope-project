#include "DspxDocument.h"
#include "DspxDocument_p.h"

#include <QUndoStack>

#include <dspxmodel/Model.h>
#include <dspxmodel/SelectionModel.h>
#include <dspxmodel/UndoableModelStrategy.h>

#include <transactional/TransactionController.h>
#include <transactional/TransactionalStrategy.h>

namespace Core {

    class TransactionalModelStrategy : public TransactionalStrategy {
    public:
        explicit TransactionalModelStrategy(dspx::UndoableModelStrategy *undoableModelStrategy, QObject *parent = nullptr)
            : TransactionalStrategy(parent), m_strategy(undoableModelStrategy) {
        }

        void beginTransaction() override {
            m_strategy->undoStack()->beginMacro("");
        }
        void abortTransaction() override {
            m_strategy->undoStack()->endMacro();
            m_strategy->undoStack()->undo();
        }
        void commitTransaction() override {
            m_strategy->undoStack()->endMacro();
        }
        void moveCurrentStepBy(int count) override {
            m_strategy->undoStack()->setIndex(m_strategy->undoStack()->index() + count);
        }

    private:
        dspx::UndoableModelStrategy *m_strategy;

    };

    DspxDocument::DspxDocument(QObject *parent) : QObject(parent), d_ptr(new DspxDocumentPrivate) {
        Q_D(DspxDocument);
        d->q_ptr = this;
        auto modelStrategy = new dspx::UndoableModelStrategy; // TODO use substate in future
        d->model = new dspx::Model(modelStrategy, this);
        modelStrategy->setParent(d->model);
        d->selectionModel = new dspx::SelectionModel(d->model, this);
        auto transactionalStrategy = new TransactionalModelStrategy(modelStrategy);
        d->transactionController = new TransactionController(transactionalStrategy, this);
        transactionalStrategy->setParent(d->transactionController);
    }

    DspxDocument::~DspxDocument() = default;

    dspx::Model *DspxDocument::model() const {
        Q_D(const DspxDocument);
        return d->model;
    }

    dspx::SelectionModel *DspxDocument::selectionModel() const {
        Q_D(const DspxDocument);
        return d->selectionModel;
    }

    TransactionController *DspxDocument::transactionController() const {
        Q_D(const DspxDocument);
        return d->transactionController;
    }

}

#include "moc_DspxDocument.cpp"
