

#include "./fb-bindings-transaction.h"
#include "./fb-bindings-connection.h"

bool Transaction::commit_transaction()
{
	if (isc_commit_transaction(status, &trans))
	{
		trans = 0;
		return false;
	}
	trans = 0;
	return true;
}

bool Transaction::rollback_transaction()
{
	if (isc_rollback_transaction(status, &trans))
	{
		trans = 0;
		return false;
	}
	trans = 0;
	return true;
}

bool Transaction::start_transaction()
{
	if (!trans)
	{
		if (isc_start_transaction(status, &trans, 1, &connection->db, 0, NULL))
		{
			trans = 0;
			ERR_MSG(this, Transaction);
			return false;
		}
		return true;
	}

	strncpy(err_message, "Old transaction should be commited or rolled back.", MAX_ERR_MSG_LEN);
	return false;
}

void Transaction::EIO_After_TransactionRequest(uv_work_t *req)
{
	//  uv_unref(uv_default_loop());
	Nan::HandleScope scope;
	struct transaction_request *tr_req = (struct transaction_request *)(req->data);
	delete req;
	Local<Value> argv[1];

	if (!tr_req->result) {
		argv[0] = Nan::Error(*Nan::Utf8String(ERR_MSG(tr_req->transaction, Connection)));
	}
	else {
		argv[0] = Nan::Null();
	}

	Nan::TryCatch try_catch;

	tr_req->callback->Call(1, argv);

	if (try_catch.HasCaught()) {
		Nan::FatalException(try_catch);
	}

	tr_req->transaction->stop_async();
	tr_req->transaction->Unref();
	free(tr_req);


}

void Transaction::EIO_TransactionRequest(uv_work_t *req)
{
	struct transaction_request *tr_req = (struct transaction_request *)(req->data);
	switch (tr_req->type) {
	case rCommit:
		tr_req->result = tr_req->transaction->commit_transaction();
		break;
	case rRollback:
		tr_req->result = tr_req->transaction->rollback_transaction();
		break;
	case rStart:
		tr_req->result = tr_req->transaction->start_transaction();
	}
	return;
}

void  Transaction::makeTrRequest(const Nan::FunctionCallbackInfo<v8::Value>& info, TransReqType type) {
	struct transaction_request *tr_req =
		(struct transaction_request *)calloc(1, sizeof(struct transaction_request));
	if (!tr_req) {
		Nan::LowMemoryNotification();
		return Nan::ThrowError("Could not allocate memory.");
		
	}

	if (info.Length() < 1) {
		return Nan::ThrowError("Expecting Callback Function argument");
	}
	tr_req->transaction = this;
	tr_req->callback = new Nan::Callback(Local<Function>::Cast(info[0]));
	tr_req->type = type;

	tr_req->transaction->start_async();

	uv_work_t* req = new uv_work_t();
	req->data = tr_req;
	uv_queue_work(uv_default_loop(), req, EIO_TransactionRequest, (uv_after_work_cb)EIO_After_TransactionRequest);
	
	tr_req->transaction->Ref();
	
}

void Transaction::InstCommit(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
	makeTrRequest(info, rCommit);
}

NAN_METHOD(Transaction::Commit)
{
	Nan::ObjectWrap::Unwrap<Transaction>(info.This())->InstCommit(info);
}

void Transaction::InstRollback(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
	makeTrRequest(info, rRollback);
}

NAN_METHOD(Transaction::Rollback)
{
	Nan::ObjectWrap::Unwrap<Transaction>(info.This())->InstRollback(info);
}

void Transaction::InstStart(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
	makeTrRequest(info, rStart);
}

NAN_METHOD(Transaction::Start)
{
	Nan::ObjectWrap::Unwrap<Transaction>(info.This())->InstStart(info);
}

Transaction::Transaction(Connection *conn) {
	connection = conn;
	trans = NULL;
}
Transaction::~Transaction() {
	if (trans) {
		commit_transaction();
	}
}
