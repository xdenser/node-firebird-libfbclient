/*!
 * Copyright by Denys Khanzhiyev aka xdenser
 *
 * See license text in LICENSE file
 */
#define BUILDING_NODE_EXTENSION 1
#include "./fb-bindings-statement.h" 


Nan::Persistent<FunctionTemplate> FBStatement::constructor_template;

void
 FBStatement::Initialize (v8::Local<v8::Object> target)
 {
	Nan::HandleScope scope;
    
    Local<FunctionTemplate> t =  Nan::New<FunctionTemplate>(FBStatement::New);
    constructor_template.Reset(t);

    t->Inherit(Nan::New(FBResult::constructor_template));
    t->SetClassName(Nan::New("FBStatement").ToLocalChecked());

    Local<ObjectTemplate> instance_template =
        t->InstanceTemplate();
        
    Nan::SetPrototypeMethod(t, "execSync", ExecSync);
	Nan::SetPrototypeMethod(t, "execInTransSync", ExecInTransSync);
    Nan::SetPrototypeMethod(t, "exec", Exec);
	Nan::SetPrototypeMethod(t, "execInTrans", ExecInTrans);


    instance_template->SetInternalFieldCount(1);
    
    Nan::Set(target, Nan::New("FBStatement").ToLocalChecked(), Nan::GetFunction(t).ToLocalChecked());
   
 }
 
NAN_METHOD(FBStatement::New)
  {
    Nan::HandleScope scope;
    
    REQ_EXT_ARG(0, js_in_sqldap);
    REQ_EXT_ARG(1, js_out_sqldap);
    REQ_EXT_ARG(2, js_stmtp);
    REQ_EXT_ARG(3, js_connection);
    
    XSQLDA *insqldap;
    insqldap = static_cast<XSQLDA *>(js_in_sqldap->Value());
    XSQLDA *outsqldap;
    outsqldap = static_cast<XSQLDA *>(js_out_sqldap->Value());
    isc_stmt_handle *astmtp;
    astmtp = static_cast<isc_stmt_handle *>(js_stmtp->Value()); 
    Connection  *conn;
    conn = static_cast<Connection *>(js_connection->Value()); 
    
    FBStatement *res = new FBStatement(insqldap,outsqldap, astmtp, conn);
    res->Wrap(info.This());

    info.GetReturnValue().Set(info.This());
  }

void FBStatement::InstExecSync(const Nan::FunctionCallbackInfo<v8::Value>& info, Transaction* transaction, int firstArg)
 { 
    Nan::HandleScope scope;
    
    FBResult::set_params(in_sqlda, info, firstArg);

    if(retres) 
    {
      isc_dsql_free_statement(status, &stmt, DSQL_close);
      if (status[1])
      {
        return Nan::ThrowError(
           String::Concat(FB_MAYBE_NEED_ISOLATE Nan::New("In FBStatement::execSync, free_statement - ").ToLocalChecked(),ERR_MSG(this, FBStatement)));
      }
    }

	Transaction *tr = transaction;

	if (!connection->check_trans(&tr)) {
		return Nan::ThrowError("Failed to get default transaction"); 
	}
    
  
	if (isc_dsql_execute2(status, &tr->trans, &stmt, SQL_DIALECT_V6, in_sqlda,  statement_type == isc_info_sql_stmt_select ? NULL:sqldap))
        {
	      return Nan::ThrowError(
	             String::Concat(FB_MAYBE_NEED_ISOLATE Nan::New("In FBStatement::execSync - ").ToLocalChecked(),ERR_MSG(this, FBStatement)));
        }
        
        if(sqldap->sqld && statement_type != isc_info_sql_stmt_select){ 
           Local<Object> js_result_row;   
           js_result_row = getCurrentRow(true);
           info.GetReturnValue().Set(js_result_row);
        }  
    
    
    if(!sqldap->sqld) 
          return;

    retres = true;
    
    return;
    
 }

NAN_METHOD(FBStatement::ExecSync)
{
	Nan::HandleScope scope;

	FBStatement *fb_stmt = Nan::ObjectWrap::Unwrap<FBStatement>(info.This());
	fb_stmt->InstExecSync(info, NULL, 0);
}

NAN_METHOD(FBStatement::ExecInTransSync)
{
	Nan::HandleScope scope;

	if (info.Length() < 1) {
		Nan::ThrowError("At least 1 argument expected");
		return;
	}

	Local<v8::Object> handle = Nan::To<v8::Object>(info[0]).ToLocalChecked();

	if (!Nan::New(Transaction::constructor_template)->HasInstance(handle)) {
		Nan::ThrowError("Tansaction expected as first argument");
		return;
	}
	
	FBStatement *fb_stmt = Nan::ObjectWrap::Unwrap<FBStatement>(info.This());
	Transaction *tr = Nan::ObjectWrap::Unwrap<Transaction>(Nan::To<v8::Object>(info[0]).ToLocalChecked());
	fb_stmt->InstExecSync(info, tr, 1);
}
 
void FBStatement::EIO_After_Exec(uv_work_t *req)
 {
   Nan::HandleScope scope;
   
   struct exec_request *e_req = (struct exec_request *)(req->data);
   delete req;
   FBStatement *fb_stmt = e_req->statement;
   Local<Value> argv[3];
   Local<String> event;
   int argc = 0;
   if(!e_req->result)
   {
     argv[0] = Nan::Error(*Nan::Utf8String(
         String::Concat(FB_MAYBE_NEED_ISOLATE Nan::New("In FBStatement::EIO_After_Exec - ").ToLocalChecked(),ERR_MSG(fb_stmt, FBStatement))));
     argc = 0;
     event = Nan::New("error").ToLocalChecked();     
   }
   else
   {
	   argv[0] = Nan::Null();
	   argc = 1;
	   event = Nan::New("result").ToLocalChecked();
	   if (fb_stmt->sqldap->sqld) {
		   fb_stmt->retres = true;
		   if (fb_stmt->statement_type != isc_info_sql_stmt_select)
		   {
			   argv[1] = fb_stmt->getCurrentRow(true);
			   argc = 2;
		   }
	   }
   }
   ((FBEventEmitter*) fb_stmt)->Emit(event,argc,argv);           
    fb_stmt->stop_async();
    fb_stmt->Unref();
    free(e_req);
 }
    
void FBStatement::EIO_Exec(uv_work_t *req)
 {
    struct exec_request *e_req = (struct exec_request *)(req->data);
    FBStatement *fb_stmt = e_req->statement;

	Transaction *tr = e_req->trans;

	if (!fb_stmt->connection->check_trans(&tr)) {
		return Nan::ThrowError("Failed to get default transaction");
	}
    
 
	if (isc_dsql_execute2(fb_stmt->status, &tr->trans, &fb_stmt->stmt, SQL_DIALECT_V6, fb_stmt->in_sqlda, fb_stmt->statement_type == isc_info_sql_stmt_select ? NULL:fb_stmt->sqldap))
	  e_req->result = false;
        else 
	  e_req->result = true;
 
    
    return ;
 }
 
void FBStatement::InstExec(const Nan::FunctionCallbackInfo<v8::Value>& info, Transaction* transaction, int firstArg)
 {
    Nan::HandleScope scope;
  
    struct exec_request *e_req =
         (struct exec_request *)calloc(1, sizeof(struct exec_request));

    if (!e_req) {
      Nan::LowMemoryNotification();
      return Nan::ThrowError("Could not allocate memory.");
    }
    
    FBResult::set_params(in_sqlda, info, firstArg);
    if(retres) 
    {
      isc_dsql_free_statement(status, &stmt, DSQL_close);
      if (status[1])
      {
        return Nan::ThrowError(
           String::Concat(FB_MAYBE_NEED_ISOLATE Nan::New("In FBStatement::exec, free_statement - ").ToLocalChecked(),ERR_MSG(this, FBStatement)));
      }
    }
    
    e_req->statement = this;
	e_req->trans = transaction;
    
    start_async();

	uv_work_t* req = new uv_work_t();
    req->data = e_req;
    uv_queue_work(uv_default_loop(), req, EIO_Exec,  (uv_after_work_cb)EIO_After_Exec);
    Ref();
 }

NAN_METHOD(FBStatement::Exec)
{
	Nan::HandleScope scope;
	FBStatement *fb_stmt = Nan::ObjectWrap::Unwrap<FBStatement>(info.This());
	fb_stmt->InstExec(info, NULL, 0);
}

NAN_METHOD(FBStatement::ExecInTrans)
{
	Nan::HandleScope scope;

	if (info.Length() < 1) {
		Nan::ThrowError("At least 1 argument expected");
		return;
	}

	Local<v8::Object> handle = Nan::To<v8::Object>(info[0]).ToLocalChecked();
	if (!Nan::New(Transaction::constructor_template)->HasInstance(handle)) {
		Nan::ThrowError("Tansaction expected as first argument");
		return;
	}

	FBStatement *fb_stmt = Nan::ObjectWrap::Unwrap<FBStatement>(info.This());
	Transaction *tr = Nan::ObjectWrap::Unwrap<Transaction>(handle);
	fb_stmt->InstExec(info, tr, 1);
}


  
 FBStatement::FBStatement(XSQLDA *insqlda, XSQLDA *outsqlda, isc_stmt_handle *astmtp, Connection* aconn) : 
              FBResult (outsqlda,astmtp,aconn)
 {
//   conn = aconn;
   in_sqlda = insqlda;
 //  out_sqlda = outsqlda;
 //  stmt = *astmtp;
   retres = false;
   strncpy(cursor, "test_cursor", sizeof(cursor));
// Get sql info
   short l;
   static char     stmt_info[] = { isc_info_sql_stmt_type };
   char            info_buffer[20];
   
   if (!isc_dsql_sql_info(status, astmtp, sizeof (stmt_info), stmt_info, 
              sizeof (info_buffer), info_buffer))
    {
        l = (short) isc_vax_integer((char *) info_buffer + 1, 2);
        statement_type = isc_vax_integer((char *) info_buffer + 3, l);
    }
} 
 
 FBStatement::~FBStatement()
 {
   if(in_sqlda) {
     FBResult::clean_sqlda(in_sqlda);
     free(in_sqlda);
   }
 }
 

