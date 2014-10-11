/*!
 * Copyright by Denys Khanzhiyev aka xdenser
 *
 * See license text in LICENSE file
 */
#define BUILDING_NODE_EXTENSION 1
#include "./fb-bindings-statement.h" 


Persistent<FunctionTemplate> FBStatement::constructor_template;

void
 FBStatement::Initialize (v8::Handle<v8::Object> target)
 {
	NanScope();
    
    Local<FunctionTemplate> t =  NanNew<FunctionTemplate>(FBStatement::New);
    NanAssignPersistent(constructor_template,t);

    t->Inherit(NanNew(FBResult::constructor_template));
    t->SetClassName(NanNew("FBStatement"));

    Local<ObjectTemplate> instance_template =
        t->InstanceTemplate();
        
    NODE_SET_PROTOTYPE_METHOD(t, "execSync", ExecSync);
    NODE_SET_PROTOTYPE_METHOD(t, "exec", Exec);


    instance_template->SetInternalFieldCount(1);
    
    target->Set(NanNew("FBStatement"), t->GetFunction());
   
 }
 
NAN_METHOD(FBStatement::New)
  {
    NanScope();
    
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
    res->Wrap(args.This());

    NanReturnValue(args.This());
  }

NAN_METHOD(FBStatement::ExecSync)
 { 
    NanScope();
    
    FBStatement *fb_stmt = ObjectWrap::Unwrap<FBStatement>(args.This());
    
    FBResult::set_params(fb_stmt->in_sqlda, args);

    if(fb_stmt->retres) 
    {
      isc_dsql_free_statement(fb_stmt->status, &fb_stmt->stmt, DSQL_close);
      if (fb_stmt->status[1])
      {
        return NanThrowError(
           String::Concat(NanNew("In FBStatement::execSync, free_statement - "),ERR_MSG(fb_stmt, FBStatement)));
      }
    }
    
    if(fb_stmt->statement_type == isc_info_sql_stmt_select)
    {    
	if (isc_dsql_execute(fb_stmt->status, &fb_stmt->connection->trans, &fb_stmt->stmt, SQL_DIALECT_V6, fb_stmt->in_sqlda))
        {
	      return NanThrowError(
	             String::Concat(NanNew("In FBStatement::execSync - "),ERR_MSG(fb_stmt, FBStatement)));
        }
        
    }
    else
    {
	if (isc_dsql_execute2(fb_stmt->status, &fb_stmt->connection->trans, &fb_stmt->stmt, SQL_DIALECT_V6, fb_stmt->in_sqlda,  fb_stmt->sqldap))
        {
	      return NanThrowError(
	             String::Concat(NanNew("In FBStatement::execSync - "),ERR_MSG(fb_stmt, FBStatement)));
        }
        
        if(fb_stmt->sqldap->sqld){ 
           Local<Object> js_result_row;   
           js_result_row = fb_stmt->getCurrentRow(true);
           NanReturnValue(js_result_row);
        }  
    
    }    
    
    
    if(!fb_stmt->sqldap->sqld) 
          NanReturnUndefined();

    fb_stmt->retres = true;
    
    NanReturnUndefined();
    
 }
 
void FBStatement::EIO_After_Exec(uv_work_t *req)
 {
   NanScope();
   
   struct exec_request *e_req = (struct exec_request *)(req->data);
   delete req;
   FBStatement *fb_stmt = e_req->statement;
   Local<Value> argv[3];
   Handle<String> event;
   int argc = 0;
   if(!e_req->result)
   {
     argv[0] = NanError(*NanAsciiString(
         String::Concat(NanNew("In FBStatement::EIO_After_Exec - "),ERR_MSG(fb_stmt, FBStatement))));
     argc = 0;
     event = NanNew("error");     
   }
   else
   {
	if(!fb_stmt->sqldap->sqld) 
	{
    	    argc = 1; 
    	    event = NanNew("result");     
    	    argv[0] = NanNull();	
        }
        else
        {
	        argv[0] = NanNull();	
	        argc = 1;      
    		event = NanNew("result");                             
     
          if(fb_stmt->statement_type != isc_info_sql_stmt_select) 
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
    
    if(fb_stmt->statement_type == isc_info_sql_stmt_select)
    {
        if (isc_dsql_execute(fb_stmt->status, &fb_stmt->connection->trans, &fb_stmt->stmt, SQL_DIALECT_V6, fb_stmt->in_sqlda))
	  e_req->result = false;
        else 
	  e_req->result = true;
    }
    else
    {
	if (isc_dsql_execute2(fb_stmt->status, &fb_stmt->connection->trans, &fb_stmt->stmt, SQL_DIALECT_V6, fb_stmt->in_sqlda,  fb_stmt->sqldap))
	  e_req->result = false;
        else 
	  e_req->result = true;
    }	  
    
    return ;
 }
 
NAN_METHOD(FBStatement::Exec)
 {
    NanScope();
    FBStatement *fb_stmt = ObjectWrap::Unwrap<FBStatement>(args.This());
    
    struct exec_request *e_req =
         (struct exec_request *)calloc(1, sizeof(struct exec_request));

    if (!e_req) {
      V8::LowMemoryNotification();
      return NanThrowError("Could not allocate memory.");
    }
    
    FBResult::set_params(fb_stmt->in_sqlda, args);
    if(fb_stmt->retres) 
    {
      isc_dsql_free_statement(fb_stmt->status, &fb_stmt->stmt, DSQL_close);
      if (fb_stmt->status[1])
      {
        return NanThrowError(
           String::Concat(NanNew("In FBStatement::exec, free_statement - "),ERR_MSG(fb_stmt, FBStatement)));
      }
    }
    
    e_req->statement = fb_stmt;
    
    fb_stmt->start_async();

	uv_work_t* req = new uv_work_t();
    req->data = e_req;
    uv_queue_work(uv_default_loop(), req, EIO_Exec,  (uv_after_work_cb)EIO_After_Exec);

    
   // uv_ref(uv_default_loop());
    fb_stmt->Ref();
    
    NanReturnUndefined();
 
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
  // printf("fbstatement destructor !\n");
 /*  if(out_sqlda) {
     FBResult::clean_sqlda(out_sqlda);
     free(out_sqlda);
   } */
 }
 

