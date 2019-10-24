

#include "./fb-bindings-transaction.h"
#include "./fb-bindings-connection.h"

Nan::Persistent<FunctionTemplate> Transaction::constructor_template;

void Transaction::Initialize(v8::Local<v8::Object> target) {
	Local<FunctionTemplate> t = Nan::New<FunctionTemplate>(Transaction::New);
	constructor_template.Reset(t);

	t->Inherit(Nan::New(FBEventEmitter::constructor_template));
	t->SetClassName(Nan::New("Transaction").ToLocalChecked());

	Local<ObjectTemplate> instance_template = t->InstanceTemplate();

	Nan::SetPrototypeMethod(t, "commit", Commit);
	Nan::SetPrototypeMethod(t, "rollback", Rollback);
	Nan::SetPrototypeMethod(t, "start", Start);

	Nan::SetPrototypeMethod(t, "commitSync", CommitSync);
	Nan::SetPrototypeMethod(t, "rollbackSync", RollbackSync);
	Nan::SetPrototypeMethod(t, "startSync", StartSync);

	Nan::SetPrototypeMethod(t, "querySync", QuerySync);
	Nan::SetPrototypeMethod(t, "query", Query);

	Nan::SetPrototypeMethod(t, "prepareSync", PrepareSync);
	
	instance_template->SetInternalFieldCount(1);

	Local<v8::ObjectTemplate> instance_t = t->InstanceTemplate();
	Nan::SetAccessor(instance_t, Nan::New("inAsyncCall").ToLocalChecked(), InAsyncGetter);
	Nan::SetAccessor(instance_t, Nan::New("inTransaction").ToLocalChecked(), InTransactionGetter);

	Nan::Set(target, Nan::New("Transaction").ToLocalChecked(), Nan::GetFunction(t).ToLocalChecked());
}

NAN_METHOD(Transaction::New) {
	Nan::HandleScope scope;

	REQ_EXT_ARG(0, js_connection);
	
	Connection  *conn;
	conn = static_cast<Connection *>(js_connection->Value());

	Transaction *res = new Transaction(conn);
	res->Wrap(info.This());

	info.GetReturnValue().Set(info.This());
}


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
	Nan::Call(*tr_req->callback, 1, argv);
	if (try_catch.HasCaught()) {
		Nan::FatalException(try_catch);
	}
        if(!tr_req->transaction->persistent().IsEmpty()) {
		tr_req->transaction->stop_async();
		tr_req->transaction->Unref();
	}
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
	
	if(!tr_req->transaction->persistent().IsEmpty()) {
		tr_req->transaction->Ref();
	}
        
	
}

void Transaction::InstCommit(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
	if (!trans) {
		return Nan::ThrowError("Transaction not started");
	}
	makeTrRequest(info, rCommit);
}

NAN_METHOD(Transaction::Commit)
{
	Nan::ObjectWrap::Unwrap<Transaction>(info.This())->InstCommit(info);
}

void Transaction::InstRollback(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
	if (!trans) {
		return Nan::ThrowError("Transaction not started");
	}
	makeTrRequest(info, rRollback);
}

NAN_METHOD(Transaction::Rollback)
{
	Nan::ObjectWrap::Unwrap<Transaction>(info.This())->InstRollback(info);
}

void Transaction::InstStart(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
	if (trans) {
		return Nan::ThrowError("Transaction already started");
	}
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

NAN_GETTER(Transaction::InTransactionGetter)
{
	Nan::HandleScope scope;
	Transaction *transaction = Nan::ObjectWrap::Unwrap<Transaction>(info.This());
	info.GetReturnValue().Set(Nan::New<Boolean>(transaction->trans));
}


NAN_METHOD(Transaction::CommitSync)
{
	Nan::HandleScope scope;
	Transaction *transaction = Nan::ObjectWrap::Unwrap<Transaction>(info.This());
	if (!transaction->trans) {
		return Nan::ThrowError("Transaction not started");
	}
	bool r = transaction->commit_transaction();
	if (!r) {
		return Nan::ThrowError(
			String::Concat(FB_MAYBE_NEED_ISOLATE Nan::New("While commitSync - ").ToLocalChecked(), ERR_MSG(transaction, Transaction)));
	}
}

NAN_METHOD(Transaction::RollbackSync)
{
	Nan::HandleScope scope;
	Transaction *transaction = Nan::ObjectWrap::Unwrap<Transaction>(info.This());
	if (!transaction->trans) {
		return Nan::ThrowError("Transaction not started");
	}

	bool r = transaction->rollback_transaction();
	if (!r) {
		return Nan::ThrowError(
			String::Concat(FB_MAYBE_NEED_ISOLATE Nan::New("While rollbackSync - ").ToLocalChecked(), ERR_MSG(transaction, Transaction)));
	}
}

NAN_METHOD(Transaction::StartSync)
{
	Nan::HandleScope scope;
	Transaction *transaction = Nan::ObjectWrap::Unwrap<Transaction>(info.This());
	if (transaction->trans) {
		return Nan::ThrowError("Transaction already started");
	}
	bool r = transaction->start_transaction();
	if (!r) {
		return Nan::ThrowError(
			String::Concat(FB_MAYBE_NEED_ISOLATE Nan::New("While startSync - ").ToLocalChecked(), ERR_MSG(transaction, Transaction)));
	}

	return;
}


NAN_METHOD(Transaction::QuerySync)
{
	Nan::HandleScope scope;
	Transaction *transaction = Nan::ObjectWrap::Unwrap<Transaction>(info.This());
	transaction->connection->InstQuerySync(info, transaction);
}

NAN_METHOD(Transaction::Query) {
	Nan::HandleScope scope;
	Transaction *transaction = Nan::ObjectWrap::Unwrap<Transaction>(info.This());
	transaction->connection->InstQuery(info, transaction);

}

NAN_METHOD(Transaction::PrepareSync) {
	Nan::HandleScope scope;
	Transaction *transaction = Nan::ObjectWrap::Unwrap<Transaction>(info.This());
	transaction->connection->InstPrepareSync(info, transaction);
}