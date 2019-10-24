#ifndef SRC_FB_BINDINGS_TRANSACTION_H_
#define SRC_FB_BINDINGS_TRANSACTION_H_


#include "./fb-bindings.h"
#include "./fb-bindings-fbeventemitter.h"

class Transaction : public FBEventEmitter {

public:

	Connection *connection;
	isc_tr_handle trans;
	ISC_STATUS_ARRAY status;

	static Nan::Persistent<FunctionTemplate> constructor_template;
	static void	Initialize(v8::Local<v8::Object> target);
	static NAN_METHOD(New);

	bool commit_transaction();

	bool rollback_transaction();

	bool start_transaction();

	enum TransReqType {
		rCommit,
		rRollback,
		rStart
	};

	struct transaction_request {
		Nan::Callback *callback;
		Transaction *transaction;
		TransReqType type;
		bool result;
	};

	Transaction(Connection *conn);
	~Transaction();

	static void EIO_After_TransactionRequest(uv_work_t *req);

	static void EIO_TransactionRequest(uv_work_t *req);

	void makeTrRequest(const Nan::FunctionCallbackInfo<v8::Value>& info, TransReqType type);

	char err_message[MAX_ERR_MSG_LEN];

	static NAN_GETTER(InTransactionGetter);

	static NAN_METHOD(Commit);
	void InstCommit(const Nan::FunctionCallbackInfo<v8::Value>& info);
	static NAN_METHOD(Rollback);
	void InstRollback(const Nan::FunctionCallbackInfo<v8::Value>& info);
	static NAN_METHOD(Start);
	void InstStart(const Nan::FunctionCallbackInfo<v8::Value>& info);

	static NAN_METHOD(CommitSync);
	static NAN_METHOD(RollbackSync);
	static NAN_METHOD(StartSync);

	static NAN_METHOD(QuerySync);
	static NAN_METHOD(Query);

	static NAN_METHOD(PrepareSync);
};
#endif