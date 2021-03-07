/*!
 * Copyright by Denys Khanzhiyev aka xdenser
 *
 * See license text in LICENSE file
 */

#include "./fb-bindings-fbresult.h"

 

 Nan::Persistent<FunctionTemplate> FBResult::constructor_template;
 
 const double FBResult::dscales[19] =  { 1, 1E1, 1E2, 1E3, 1E4, 1E5, 1E6, 1E7, 1E8, 1E9, 1E10,
                    1E11, 1E12, 1E13, 1E14, 1E15, 1E16, 1E17, 1E18 };
					
Nan::AsyncResource FBResult::asyncResource("FBResult");
					
 void
   FBResult::Initialize (v8::Local<v8::Object> target)
  {
    
    Nan::HandleScope scope;
    
    Local<FunctionTemplate> t =  Nan::New<FunctionTemplate>(FBResult::New);

    t->Inherit(Nan::New(FBEventEmitter::constructor_template));
    
    constructor_template.Reset(t);

    t->SetClassName(Nan::New("FBResult").ToLocalChecked());

    Local<ObjectTemplate> instance_template = t->InstanceTemplate();
        
    Nan::SetPrototypeMethod(t, "fetchSync", FetchSync);
    Nan::SetPrototypeMethod(t, "fetch", Fetch);
    


    instance_template->SetInternalFieldCount(1);
    
    Local<v8::ObjectTemplate> instance_t = t->InstanceTemplate();
    Nan::SetAccessor(instance_t, Nan::New("inAsyncCall").ToLocalChecked(),InAsyncGetter);
    
    Nan::Set(target, Nan::New("FBResult").ToLocalChecked(), Nan::GetFunction(t).ToLocalChecked());
  }
  
  
NAN_METHOD(FBResult::New)
  {
  // XSQLDA **asqldap, isc_stmt_handle *astmtp
    Nan::HandleScope scope;
    
    REQ_EXT_ARG(0, js_sqldap);
    REQ_EXT_ARG(1, js_stmtp);
    REQ_EXT_ARG(2, js_connection);
    
    XSQLDA *asqldap;
    asqldap = static_cast<XSQLDA *>(js_sqldap->Value());
    isc_stmt_handle *astmtp;
    astmtp = static_cast<isc_stmt_handle *>(js_stmtp->Value()); 
    Connection  *conn;
    conn = static_cast<Connection *>(js_connection->Value()); 
    
    FBResult *res = new FBResult(asqldap, astmtp, conn);
    res->Wrap(info.This());

    info.GetReturnValue().Set(info.This());
  }
  
  bool FBResult::prepare_sqlda(XSQLDA *sqlda)
  {
    int i;
    XSQLVAR* var;
    sqlda->sqln = sqlda->sqld;
    for(i = 0, var = sqlda->sqlvar; i < sqlda->sqld;i++,var++)
    {
      switch(var->sqltype & ~1)
      {
        case SQL_ARRAY:
        case SQL_BLOB:      var->sqldata = (char*) new ISC_QUAD;
                            memset(var->sqldata, 0, sizeof(ISC_QUAD));
                            break;
        case SQL_TIMESTAMP: var->sqldata = (char*) new ISC_TIMESTAMP;
                            memset(var->sqldata, 0, sizeof(ISC_TIMESTAMP));
                            break;
        case SQL_TYPE_TIME: var->sqldata = (char*) new ISC_TIME;
                            memset(var->sqldata, 0, sizeof(ISC_TIME));
                            break;                                 
        case SQL_TYPE_DATE: var->sqldata = (char*) new ISC_DATE;
                            memset(var->sqldata, 0, sizeof(ISC_DATE));
                            break;                              
        case SQL_TEXT:      var->sqldata = new char[var->sqllen + 1];
                            memset(var->sqldata, ' ', var->sqllen);
                            //memset(var->sqldata, 0, var->sqllen);
                            var->sqldata[var->sqllen] = '\0';
                            break;
        case SQL_VARYING:   var->sqldata = new char[var->sqllen + 3];
                            memset(var->sqldata, 0, 2);
                            memset(var->sqldata + 2, ' ', var->sqllen);
                            var->sqldata[var->sqllen + 2] = '\0';
                            break;
        case SQL_SHORT:     var->sqldata = (char *) new int16_t(0); break;
        case SQL_LONG:      var->sqldata = (char *) new int32_t(0); break;
        case SQL_INT64:     var->sqldata = (char *) new int64_t(0); break;
        case SQL_FLOAT:     var->sqldata = (char *) new float(0.0); break;
        case SQL_DOUBLE:    var->sqldata = (char *) new double(0.0); break;
        default:            return false;                     
      }
      if(var->sqltype & 1){
       var->sqlind = new short(-1);
      }
    }
    return true;
  }
  
  void FBResult::clean_sqlda(XSQLDA *sqlda)
  {
    int i;
    XSQLVAR* var;
    for(i = 0, var= sqlda->sqlvar; i < sqlda->sqld;i++,var++)
    {
      switch(var->sqltype & ~1)
      {
        case SQL_ARRAY:
        case SQL_BLOB:      delete (ISC_QUAD*) var->sqldata; break;
        case SQL_TIMESTAMP: delete (ISC_TIMESTAMP*) var->sqldata; break;
        case SQL_TYPE_TIME: delete (ISC_TIME*) var->sqldata; break;                                 
        case SQL_TYPE_DATE: delete (ISC_DATE*) var->sqldata; break;                              
        case SQL_TEXT:      
        case SQL_VARYING:   delete [] var->sqldata; break;
        case SQL_SHORT:     delete (int16_t *) var->sqldata; break;
        case SQL_LONG:      delete (int32_t *) var->sqldata; break;
        case SQL_INT64:     delete (int64_t *) var->sqldata; break;
        case SQL_FLOAT:     delete (float *) var->sqldata; break;
        case SQL_DOUBLE:    delete (double *) var->sqldata; break;
        default:            return;                     
      }
      if(var->sqlind != 0) delete var->sqlind;
    }
  }     
/*   
  double get_js_time(struct tm* times){
     double res = 0;
     
  }
*/
bool FBResult::clone_sqlda(XSQLDA *src_sqlda,XSQLDA **dest_sqlda)
  {
     int size = XSQLDA_LENGTH (src_sqlda->sqln);
     XSQLDA *out_sqlda = (XSQLDA *) malloc(size);
     *dest_sqlda = out_sqlda;
     if(!out_sqlda) return false;
     memcpy(out_sqlda,src_sqlda,size); 
     return prepare_sqlda(out_sqlda);
  }
  
char* errmsg1f(char* buf,const char* msg, int arg)
{
  sprintf(buf,msg,arg);
  return buf;
}  

char* errmsg2f(char* buf,const char* msg, int arg1, int arg2)
{
  sprintf(buf,msg,arg1,arg2);
  return buf;
}  

void get_date(struct tm* times, Local<Object> js_date, int* msp)
{
  Nan::HandleScope scope;
  Local<Value> val;
  
  val = Nan::Call(Local<Function>::Cast(Nan::Get( js_date, Nan::New("getFullYear").ToLocalChecked() ).ToLocalChecked()), js_date,0,NULL).ToLocalChecked();
  times->tm_year = (int) (Nan::To<int32_t>(val).FromJust()) - 1900;
  
  val = Nan::Call(Local<Function>::Cast(Nan::Get( js_date, Nan::New("getMonth").ToLocalChecked() ).ToLocalChecked()), js_date,0,NULL).ToLocalChecked();
  times->tm_mon = Nan::To<int32_t>(val).FromJust();

  val = Nan::Call(Local<Function>::Cast(Nan::Get( js_date, Nan::New("getDate").ToLocalChecked() ).ToLocalChecked()), js_date,0,NULL).ToLocalChecked();
  times->tm_mday = Nan::To<int32_t>(val).FromJust();

  val = Nan::Call(Local<Function>::Cast(Nan::Get( js_date, Nan::New("getHours").ToLocalChecked() ).ToLocalChecked()), js_date,0,NULL).ToLocalChecked();
  times->tm_hour = Nan::To<int32_t>(val).FromJust();
  
  val = Nan::Call(Local<Function>::Cast(Nan::Get( js_date, Nan::New("getMinutes").ToLocalChecked() ).ToLocalChecked()), js_date,0,NULL).ToLocalChecked();
  times->tm_min = Nan::To<int32_t>(val).FromJust();

  val = Nan::Call(Local<Function>::Cast(Nan::Get( js_date, Nan::New("getSeconds").ToLocalChecked() ).ToLocalChecked()), js_date,0,NULL).ToLocalChecked();
  times->tm_sec = Nan::To<int32_t>(val).FromJust();
  
  val = Nan::Call(Local<Function>::Cast(Nan::Get( js_date, Nan::New("getMilliseconds").ToLocalChecked() ).ToLocalChecked()), js_date,0,NULL).ToLocalChecked();
  *msp = Nan::To<int32_t>(val).FromJust();
  
}

  
void FBResult::set_params(XSQLDA *sqlda, Nan::NAN_METHOD_ARGS_TYPE info, int firstArg)
  {
    Nan::HandleScope scope;
    int i;
    XSQLVAR* var;
    PARAMVARY   *vary2;
    FBblob *blob; 
    Local<Object> obj;
    char errm[512];
   // double date_num;
    double double_val;
    struct tm  times;
    int m_sec;
    int16_t s_len;
    int64_t int_val;
//    char *txt;
	int numArgs = info.Length() - firstArg;
    if( sqlda->sqld >  numArgs) {
    	 Nan::ThrowError(errmsg2f(errm,"Expecting %d arguments, but only %d provided.",(int)sqlda->sqld, numArgs));
    	 return ;
    }
    for(i = firstArg, var = sqlda->sqlvar; i < (firstArg + sqlda->sqld);i++,var++)
    {
      
      if(info[i]->IsNull() && (var->sqltype & 1)){
       *var->sqlind = -1;
       continue;
      }
      
      switch(var->sqltype & ~1)
      { 
        case SQL_ARRAY:
        case SQL_BLOB:      
                            if(!FBblob::HasInstance(info[i])){
                            	Nan::ThrowError(errmsg1f(errm,"Expecting FBblob as argument #%d.",i+1));
                            	return ;
                            }
                            obj = info[i]->ToObject(Nan::GetCurrentContext()).ToLocalChecked();  
                            blob = Nan::ObjectWrap::Unwrap<FBblob>(obj);  
                            
                            //memcpy(dest,src,size); 
                            blob->getId((ISC_QUAD*)var->sqldata);
                            // printf("setting blob param %d,%d\n",((ISC_QUAD*)var->sqldata)->gds_quad_high,((ISC_QUAD*)var->sqldata)->gds_quad_low);
                            break;
                            
        case SQL_TIMESTAMP: 
                            if(!info[i]->IsDate()) {
                            	Nan::ThrowError(errmsg1f(errm,"Expecting Date as argument #%d.",i+1));
                            	return ;
                            }
                                                
                            get_date( &times, info[i]->ToObject(Nan::GetCurrentContext()).ToLocalChecked(), &m_sec);                            
                            isc_encode_timestamp(&times, (ISC_TIMESTAMP *)var->sqldata);
                            ((ISC_TIMESTAMP *)var->sqldata)->timestamp_time = ((ISC_TIMESTAMP *)var->sqldata)->timestamp_time + m_sec*10;
                            break;          
        case SQL_TYPE_TIME:  
                            if(!info[i]->IsDate()){
                            	Nan::ThrowError(errmsg1f(errm,"Expecting Date as argument #%d.",i+1));
                            	return ;
                            }
                            get_date( &times, info[i]->ToObject(Nan::GetCurrentContext()).ToLocalChecked(), &m_sec);                            
                            isc_encode_sql_time(&times, (ISC_TIME *)var->sqldata);
                            *((ISC_TIME *)var->sqldata) = *((ISC_TIME *)var->sqldata) + m_sec*10;
                            break;                                          
        case SQL_TYPE_DATE:  
                            if(!info[i]->IsDate()){
                            	Nan::ThrowError(errmsg1f(errm,"Expecting Date as argument #%d.",i+1));
                            	return ;
                            }
                            get_date( &times, info[i]->ToObject(Nan::GetCurrentContext()).ToLocalChecked(), &m_sec);                            
                            isc_encode_sql_date(&times, (ISC_DATE *)var->sqldata);
                            break;                               
        case SQL_TEXT:      
                            if(!info[i]->IsString()){
                            	Nan::ThrowError(errmsg1f(errm,"Expecting String as argument #%d.",i+1));
                            	return ;
                            }
                            {                           
                            Nan::Utf8String txt(info[i]->ToString(Nan::GetCurrentContext()).ToLocalChecked());  
                            s_len = (int16_t) strlen(*txt);
                            if(s_len > var->sqllen) s_len = var->sqllen;                             
                            strncpy(var->sqldata, *txt, s_len);
                            while(s_len < var->sqllen) var->sqldata[s_len++] = ' ';
                            }
			    break;

	case SQL_VARYING:		    
	                    
	                    if(!info[i]->IsString()) {
	                    	Nan::ThrowError(errmsg1f(errm,"Expecting String as  argument #%d.",i+1));
	                    	return ;
	                    }
                            {                           
                            Nan::Utf8String txt(info[i]->ToString(Nan::GetCurrentContext()).ToLocalChecked());  
                            
                            s_len = (int16_t) strlen(*txt);
                            if(s_len > var->sqllen) s_len = var->sqllen;
                            vary2 = (PARAMVARY*) var->sqldata;
                            vary2->vary_length = s_len;                            
                            strncpy((char*) vary2->vary_string, *txt, s_len);
                            }
			    break;
        case SQL_SHORT:	
                            int_val = Nan::To<int32_t>(info[i]).FromJust();
                            *(int16_t *) var->sqldata = (int16_t) int_val;
                            break;
        case SQL_LONG:      
                            int_val = Nan::To<int32_t>(info[i]).FromJust();
                            *(int32_t *) var->sqldata = (int32_t) int_val;
                            break;                     
        case SQL_INT64:                        
                            if(info[i]->IsNumber())
                            {
                              double multiplier = FBResult::dscales[-var->sqlscale];
                              double_val = Nan::To<double>(info[i]).FromMaybe(0.0);
                              *(int64_t *) var->sqldata = (int64_t) floor(double_val * multiplier + 0.5);
                            }
                            else 
                            {
                              int_val = Nan::To<int32_t>(info[i]).FromJust();
                              *(int64_t *) var->sqldata = int_val;
                            }  
                            break;                      
        case SQL_FLOAT:     
                            double_val = Nan::To<double>(info[i]).FromMaybe(0.0);
                            *(float *) var->sqldata = (float) double_val;
                            break;
        case SQL_DOUBLE:                    
                            double_val = Nan::To<double>(info[i]).FromMaybe(0.0);  
                            if( var->sqlscale != 0 )
                            {
                              double multiplier = FBResult::dscales[-var->sqlscale];
                              *(double *) var->sqldata = floor(double_val * multiplier + 0.5) / multiplier;
                            } 
                            else  *(double *) var->sqldata = double_val;
                            break;
      }
      if(var->sqltype & 1) *var->sqlind = 0;
    }
    
  }

short getCharsetSize(XSQLVAR *var){
	switch(var->sqlsubtype & 0xFF){
	 case 0: case 1: case 2: case 10: case 11: case 12: case 13: case 14:
	 case 19: case 21: case 22: case 39: case 45: case 46: case 47:
	 case 50: case 51: case 52: case 53: case 54: case 55: case 58:  return 1;
	 
	 case 5: case 6: case 8: case 44: case 56: case 57: case 64: return 2;
	 
	 case 3: return 3;
	 
	 case 4: case 59: return 4; 
	}
	return 0;
}
  
Local<Value> 
  FBResult::GetFieldValue(XSQLVAR *var, Connection* conn)
  {
    short       dtype;  
    PARAMVARY   *vary2;
  //  short       len; 
    struct tm   times;
    ISC_QUAD    bid;
    //time_t      rawtime;
  //  double      time_val;
   // int 	    days;
    short		bpc, chars; // bytes per char 
    
    
    Nan::EscapableHandleScope scope;
    
    Local<Function> con;
    Local<Value> argv[7];
    
    Local<Object> js_date;
    Local<Object> js_obj;
    Local<Value> js_field = Nan::Null();
    dtype = var->sqltype & ~1;
    if ((var->sqltype & 1) && (*var->sqlind < 0))
    {
     // NULL PROCESSING
    }
    else
    {
		switch (dtype)
		{
		case SQL_TEXT:

				if (!conn->isUTF8lctype) {
					js_field = Nan::CopyBuffer(var->sqldata, var->sqllen).ToLocalChecked();
					
					if (Nan::HasPrivate(conn->handle(), Nan::New(PROP_LC_CTYPE_DECODE).ToLocalChecked()).FromMaybe(false)) {
						argv[0] = js_field;
						js_field = Nan::Call(Local<Function>::Cast(Nan::GetPrivate(conn->handle(), Nan::New(PROP_LC_CTYPE_DECODE).ToLocalChecked()).ToLocalChecked()), conn->handle(), 1, argv).ToLocalChecked();
					}
				}
				else {

					bpc = getCharsetSize(var);
					chars = var->sqllen / (bpc != 0 ? bpc : 1);
					js_field = Nan::New<String>(var->sqldata, var->sqllen).ToLocalChecked();
					if (Local<String>::Cast(js_field)->Length() > chars)
					{
						js_obj = js_field->ToObject(Nan::GetCurrentContext()).ToLocalChecked();
						argv[0] = Nan::New<Integer>(0);
						argv[1] = Nan::New<Integer>(chars);
						js_field = asyncResource.runInAsyncScope(js_obj, Nan::New("slice").ToLocalChecked(), 2, argv).ToLocalChecked();
					}
				}
                
               //  printf(" char lengh %d/%d, %d, 1 %hx, 2 %hx, 3 %hx, 4 %hx \n",var->sqllen,Local<String>::Cast(js_field)->Length(), var->sqlsubtype, var->sqldata[0],var->sqldata[1],var->sqldata[2],var->sqldata[3]);
                //  js_field = String::New(var->sqldata);
                break;

            case SQL_VARYING:
			{
				vary2 = (PARAMVARY*)var->sqldata;
				vary2->vary_string[vary2->vary_length] = '\0';
				if (!conn->isUTF8lctype) {
					js_field = Nan::CopyBuffer((const char*)(vary2->vary_string), vary2->vary_length).ToLocalChecked();
					
					if (Nan::HasPrivate(conn->handle(), Nan::New(PROP_LC_CTYPE_DECODE).ToLocalChecked()).FromMaybe(false)) {
						argv[0] = js_field;
						js_field = Nan::Call(Local<Function>::Cast(Nan::GetPrivate(conn->handle(), Nan::New(PROP_LC_CTYPE_DECODE).ToLocalChecked()).ToLocalChecked()), conn->handle(), 1, argv).ToLocalChecked();
					}
				}
				else {
					js_field = Nan::New<String>((const char*)(vary2->vary_string)).ToLocalChecked();
				}
			}
                break;

            case SQL_SHORT:
            case SQL_LONG:
	    case SQL_INT64:
		{
		ISC_INT64	val = 0;
	//	short		field_width;
		short		dscale;
		switch (dtype)
		    {
		    case SQL_SHORT:
			val = (ISC_INT64) *(short *) var->sqldata;
		//	field_width = 6;
			break;
		    case SQL_LONG:
			val = (ISC_INT64) *(int *) var->sqldata;
		//	field_width = 11;
			break;
		    case SQL_INT64:
			val = (ISC_INT64) *(ISC_INT64 *) var->sqldata;
		//	field_width = 21;
			break;
		    }
		ISC_INT64	tens;    
		short	i;
		dscale = var->sqlscale;
		
		if (dscale < 0)
		    {
		    tens = 1;
		    for (i = 0; i > dscale; i--) tens *= 10;
                    js_field = Nan::New<Number>((double) val / tens); 
                    
		    }
		else if (dscale)
		      {
		       tens = 1;
		       for (i = 0; i < dscale; i++) tens *= 10;
		       js_field = Nan::New<Number>(double(val * tens));
	              }		    
		else
		      js_field = Nan::New<Number>((double) val);
		}
                break;

            case SQL_FLOAT:
                js_field = Nan::New<Number>(double( *(float *) (var->sqldata) ));  
                break;

            case SQL_DOUBLE:
                js_field = Nan::New<Number>(*(double *) (var->sqldata));
                break;

            case SQL_TIMESTAMP: 
	            isc_decode_timestamp((ISC_TIMESTAMP *)var->sqldata, &times);
	            js_date = Nan::New<Date>((times.tm_year - 70) * 365 * 86400000).ToLocalChecked();
	            argv[0] = Nan::New<Integer>(times.tm_year+1900);
				asyncResource.runInAsyncScope(js_date, Nan::New("setFullYear").ToLocalChecked(), 1, argv);
	            argv[0] = Nan::New<Integer>(times.tm_mon);
				asyncResource.runInAsyncScope(js_date, Nan::New("setMonth").ToLocalChecked(), 1, argv);
	            argv[0] = Nan::New<Integer>(times.tm_mday);
				asyncResource.runInAsyncScope(js_date, Nan::New("setDate").ToLocalChecked(), 1, argv);
	            argv[0] = Nan::New<Integer>(times.tm_hour);
				asyncResource.runInAsyncScope(js_date, Nan::New("setHours").ToLocalChecked(), 1, argv);
	            argv[0] = Nan::New<Integer>(times.tm_min);
				asyncResource.runInAsyncScope(js_date, Nan::New("setMinutes").ToLocalChecked(), 1, argv);
	            argv[0] = Nan::New<Integer>(times.tm_sec);
				asyncResource.runInAsyncScope(js_date, Nan::New("setSeconds").ToLocalChecked(), 1, argv);
	            argv[0] = Nan::New<Integer>(  (( ((ISC_TIMESTAMP *)var->sqldata)->timestamp_time) % 10000) / 10);
				asyncResource.runInAsyncScope(js_date, Nan::New("setMilliseconds").ToLocalChecked(), 1, argv);
	            
	            js_field = js_date;
	            break;
	            
            case SQL_TYPE_DATE:    
	            	          
	            isc_decode_sql_date((ISC_DATE *)var->sqldata, &times);
	            
	            // days from 1 jan 1858 	            
	            //days = * (int *) var->sqldata ; 
	            
	            
	            //time_val = (static_cast<double>(days) - 40587) * 86400 * 1000;
	            js_date = Nan::New<Date>((times.tm_year - 70) * 365 * 86400000).ToLocalChecked();
	           
	            
	            argv[0] = Nan::New<Integer>(times.tm_year+1900);
				asyncResource.runInAsyncScope(js_date, Nan::New("setFullYear").ToLocalChecked(), 1, argv);
		    argv[0] = Nan::New<Integer>(times.tm_mon);
				asyncResource.runInAsyncScope(js_date, Nan::New("setMonth").ToLocalChecked(), 1, argv);
	            argv[0] = Nan::New<Integer>(times.tm_mday);
				asyncResource.runInAsyncScope(js_date, Nan::New("setDate").ToLocalChecked(), 1, argv);
		   
	            argv[0] = Nan::New<Integer>(0);
				asyncResource.runInAsyncScope(js_date, Nan::New("setHours").ToLocalChecked(), 1, argv);
				asyncResource.runInAsyncScope(js_date, Nan::New("setMinutes").ToLocalChecked(), 1, argv);
				asyncResource.runInAsyncScope(js_date, Nan::New("setSeconds").ToLocalChecked(), 1, argv);
				asyncResource.runInAsyncScope(js_date, Nan::New("setMilliseconds").ToLocalChecked(), 1, argv);
	            
	            //js_field = con->Call(js_date, 3, argv);
	            /*js_date = Object::New();
	            js_date->Set(String::New("year"),
                             Integer::New(times.tm_year+1900));
                    js_date->Set(String::New("month"),
                             Integer::New(times.tm_mon+1));
                    js_date->Set(String::New("day"),
                             Integer::New(times.tm_mday));
                             
	            
	            js_date->Set(String::New("date"),
                             Date::New(time_val));*/
	            js_field = js_date;
	            break;
	            
           case SQL_TYPE_TIME:    
	            isc_decode_sql_time((ISC_TIME *)var->sqldata, &times);
	            
	            js_date = Nan::New<Date>((times.tm_year - 70) * 365 * 86400000).ToLocalChecked();
	      	    argv[0] = Nan::New<Integer>(times.tm_year+1900);
				asyncResource.runInAsyncScope(js_date, Nan::New("setFullYear").ToLocalChecked(), 1, argv);
	      	    argv[0] = Nan::New<Integer>(times.tm_mon);
				asyncResource.runInAsyncScope(js_date, Nan::New("setMonth").ToLocalChecked(), 1, argv);
	      	    argv[0] = Nan::New<Integer>(times.tm_mday);
				asyncResource.runInAsyncScope(js_date, Nan::New("setDate").ToLocalChecked(), 1, argv);
	      	    argv[0] = Nan::New<Integer>(times.tm_hour);
				asyncResource.runInAsyncScope(js_date, Nan::New("setHours").ToLocalChecked(), 1, argv);
	      	    argv[0] = Nan::New<Integer>(times.tm_min);
				asyncResource.runInAsyncScope(js_date, Nan::New("setMinutes").ToLocalChecked(), 1, argv);
	      	    argv[0] = Nan::New<Integer>(times.tm_sec);
				asyncResource.runInAsyncScope(js_date, Nan::New("setSeconds").ToLocalChecked(), 1, argv);
	      	    argv[0] = Nan::New<Integer>(  (( ((ISC_TIMESTAMP *)var->sqldata)->timestamp_time) % 10000) / 10);
				asyncResource.runInAsyncScope(js_date, Nan::New("setMilliseconds").ToLocalChecked(), 1, argv);
	            js_field = js_date;
	            break;            

            case SQL_BLOB:
            case SQL_ARRAY:
                /* Print the blob id on blobs or arrays */
                bid = *(ISC_QUAD *) var->sqldata;
                
                argv[0] = Nan::New<External>(&bid);
		        argv[1] = Nan::New<External>(conn);
				Local<Object> js_blob = Nan::NewInstance(Nan::GetFunction(Nan::New(FBblob::constructor_template)).ToLocalChecked(), 2, argv).ToLocalChecked();

                js_field = js_blob;
                break;

        }

    }
    
    return scope.Escape(js_field);
  }
  
Local<Object> 
  FBResult::getCurrentRow(bool asObject)
  {
    short  i, num_cols;
   
    Nan::EscapableHandleScope scope;
    Local<Object> js_result_row;   
    Local<Value> js_field;
    
    if(!sqldap) num_cols = 0;
    else num_cols = sqldap->sqld;
 
    
    if(asObject)
            js_result_row = Nan::New<Object>();
         else 
            js_result_row = Nan::New<Array>();
        
    for (i = 0; i < num_cols; i++)
    {
            js_field = FBResult::GetFieldValue((XSQLVAR *) &sqldap->sqlvar[i], connection);
            if(asObject)
            { 
            	Nan::Set(js_result_row, Nan::New<String>(sqldap->sqlvar[i].aliasname,sqldap->sqlvar[i].aliasname_length).ToLocalChecked(), js_field);
            	//js_result_row->Set(String::New(sqldap->sqlvar[i].sqlname,sqldap->sqlvar[i]), js_field);
            }
            else
				Nan::Set(js_result_row, Nan::New<Integer>(i), js_field);
    }    
    
    return scope.Escape(js_result_row);
  }  

NAN_METHOD(FBResult::FetchSync)
  {
  
    Nan::HandleScope scope;
    
    FBResult *fb_res = Nan::ObjectWrap::Unwrap<FBResult>(info.This());
    
    ISC_STATUS     fetch_stat;
    XSQLDA         *sqlda;
    
    uint32_t       j = 0;
    sqlda = fb_res->sqldap;
    if(!sqlda){ return;}
    
    Local<Value> js_result = Nan::Null();
    
    if (info.Length() < 2){
       return Nan::ThrowError("Expecting 2 arguments");
    }
    
    int32_t rowCount = -1;
    if(info[0]->IsInt32()){
       rowCount = (int32_t) info[0]->Int32Value(Nan::GetCurrentContext()).FromJust();
    }
    else if(! (info[0]->IsString() && info[0]->Equals(Nan::GetCurrentContext(), Nan::New("all").ToLocalChecked()).FromMaybe(false))){
       return Nan::ThrowError("Expecting integer or string as first argument");
    };
    if(rowCount<=0) rowCount = -1;
    
    bool rowAsObject = false;
    if(info[1]->IsBoolean()){
         rowAsObject = Nan::To<bool>(info[1]).FromMaybe(false);
    }else if(info[1]->IsString() && info[1]->Equals(Nan::GetCurrentContext(), Nan::New("array").ToLocalChecked()).FromMaybe(false)){
         rowAsObject = false;
    }else if(info[1]->IsString() && info[1]->Equals(Nan::GetCurrentContext(), Nan::New("object").ToLocalChecked()).FromMaybe(false)){
         rowAsObject = true;
    } else{
     return Nan::ThrowError("Expecting bool or string('array'|'object') as second argument");
    };   
    
    Local<Value> js_field;
    Local<Object> js_result_row;   
        
    Local<Array> res = Nan::New<Array>(); 
    while (((fetch_stat = isc_dsql_fetch(fb_res->status, &fb_res->stmt, SQL_DIALECT_V6, sqlda)) == 0)&&((rowCount==-1)||(rowCount>0)))
    {
        js_result_row = fb_res->getCurrentRow(rowAsObject);
    /*     if(rowAsObject)
            js_result_row = Object::New();
         else 
            js_result_row = Array::New();
        
        for (i = 0; i < num_cols; i++)
        {
            js_field = FBResult::GetFieldValue((XSQLVAR *) &sqlda->sqlvar[i], fb_res->connection);
            if(rowAsObject)
            { 
              js_result_row->Set(String::New(sqlda->sqlvar[i].sqlname), js_field);
            }
            else
            js_result_row->Set(Integer::NewFromUnsigned(i), js_field);
        }*/
        Nan::Set(res, Nan::New<Uint32>(j++), js_result_row);
        if(rowCount>0) rowCount--;
    }
    
    if ((fetch_stat != 100L) && fetch_stat) 
          return Nan::ThrowError(
            String::Concat(FB_MAYBE_NEED_ISOLATE Nan::New("While FetchSync - ").ToLocalChecked(),ERR_MSG(fb_res,FBResult)));

    
//    if(j==1) js_result = res->Get(0);
//    else 
    js_result = res;
    
    info.GetReturnValue().Set(js_result);
    if(rowCount==-1) {
       fb_res->finished();
    }
  }

void FBResult::EIO_After_Fetch(uv_work_t *req)
  {
   // uv_unref(uv_default_loop());
    Nan::HandleScope scope;
    struct fetch_request *f_req = (struct fetch_request *)(req->data);
	delete req;
   // short i, num_cols;
   // num_cols = f_req->res->sqldap->sqld;
    
    Local<Value> js_field;
    Local<Object> js_result_row;   
    Local<Value> argv[3];
    int argc = 0;
    
    if(f_req->result) {
        if(f_req->rowCount>0) f_req->rowCount--;

        js_result_row = f_req->res->getCurrentRow(f_req->rowAsObject);
        /*
	if(f_req->rowAsObject)
    	    js_result_row = Object::New();
	else 
    	    js_result_row = Array::New();
    
	for (i = 0; i < num_cols; i++)
	{
    	    js_field = FBResult::GetFieldValue((XSQLVAR *) &f_req->res->sqldap->sqlvar[i], f_req->res->connection);
    	    if(f_req->rowAsObject)
    	    { 
        	js_result_row->Set(String::New(f_req->res->sqldap->sqlvar[i].sqlname), js_field);
    	    }
    	    else
        	js_result_row->Set(Integer::NewFromUnsigned(i), js_field);
        }
        */
        if (f_req->rowCallback) {
    	    argv[0] = js_result_row;

    	    Nan::TryCatch try_catch;

//	        Local<Value> ret = Nan::New(f_req->rowCallback->Call(1, argv));
	        Local<Value> ret = Nan::Call(*f_req->rowCallback, 1, argv).ToLocalChecked();

	        if (try_catch.HasCaught()) {
				Nan::FatalException(try_catch);
    	    } else if ((!ret->IsBoolean() || Nan::To<bool>(ret).FromMaybe(false))&&f_req->rowCount!=0) {
	    		uv_work_t* req = new uv_work_t();
                req->data = f_req;
                uv_queue_work(uv_default_loop(), req, EIO_Fetch,  (uv_after_work_cb)EIO_After_Fetch);


            	//uv_ref(uv_default_loop());
            	return;
	        } else {
                argc = 2;
                argv[0] = Nan::Null();
                argv[1] = Nan::False();
            }
        } else {
            /* TODO:
             *   accumulate result here
             */
        }
    } else if (f_req->fetchStat!=100L) {
        argc = 1;
        argv[0] = Nan::Error(*Nan::Utf8String(
        String::Concat(FB_MAYBE_NEED_ISOLATE Nan::New("While fetching - ").ToLocalChecked(),ERR_MSG(f_req->res,FBResult))));
    } else {
        argc = 2;
        argv[0] = Nan::Null();
        argv[1] = Nan::True();
    }

	Nan::TryCatch try_catch;
    Nan::Call(*f_req->eofCallback, argc, argv);

    if (try_catch.HasCaught()) {
		Nan::FatalException(try_catch);
    }

    f_req->res->stop_async();
    f_req->res->Unref();
    free(f_req);
    if(f_req->rowCount==-1) {
        f_req->res->finished();
    }

}

void FBResult::EIO_Fetch(uv_work_t *req)
  {
    struct fetch_request *f_req = (struct fetch_request *)(req->data);
    
    f_req->fetchStat = isc_dsql_fetch(f_req->res->status, &f_req->res->stmt, SQL_DIALECT_V6, f_req->res->sqldap);
    
    f_req->result = (f_req->fetchStat == 0);

    return ;
  }
  
NAN_METHOD(FBResult::Fetch)
  {
    Nan::HandleScope scope;
    FBResult *res = Nan::ObjectWrap::Unwrap<FBResult>(info.This());
    
    struct fetch_request *f_req =
         (struct fetch_request *)calloc(1, sizeof(struct fetch_request));

    if (!f_req) {
      Nan::LowMemoryNotification();
      return Nan::ThrowError("Could not allocate memory.");
    }
    
    if (info.Length() != 4){
       return Nan::ThrowError("Expecting 4 arguments");
    }
    
    f_req->rowCount = -1;
    if(info[0]->IsInt32()){
       f_req->rowCount = (int) info[0]->Int32Value(Nan::GetCurrentContext()).FromJust();
    }
    else if(! (info[0]->IsString() && info[0]->Equals(Nan::GetCurrentContext(), Nan::New("all").ToLocalChecked()).FromJust())){
       return Nan::ThrowError("Expecting integer or string as first argument");
    };
    if(f_req->rowCount<=0) f_req->rowCount = -1;
    
    f_req->rowAsObject = false;
    if(info[1]->IsBoolean()){
         f_req->rowAsObject = Nan::To<bool>(info[1]).FromMaybe(false);
    }else if(info[1]->IsString() && info[1]->Equals(Nan::GetCurrentContext(), Nan::New("array").ToLocalChecked()).FromJust()){
         f_req->rowAsObject = false;
    }else if(info[1]->IsString() && info[1]->Equals(Nan::GetCurrentContext(), Nan::New("object").ToLocalChecked()).FromJust()){
         f_req->rowAsObject = true;
    } else{
     return Nan::ThrowError("Expecting bool or string('array'|'object') as second argument");
    };
    
/*    if(!info[2]->IsFunction())  f_req->rowCallback = Nan::Persistent<Value>::New(Null());
    else  f_req->rowCallback = Nan::Persistent<Function>::New(Local<Function>::Cast(info[2]));
*/   
    if(info[2]->IsFunction()) f_req->rowCallback  = new Nan::Callback(Local<Function>::Cast(info[2]));
    else f_req->rowCallback = NULL; 
    
    if(!info[3]->IsFunction()) {
      return Nan::ThrowError("Expecting Function as fourth argument");
    }

    
    f_req->res = res;
    f_req->eofCallback = new Nan::Callback(Local<Function>::Cast(info[3]));
    
    res->start_async();

	uv_work_t* req = new uv_work_t();
    req->data = f_req;
    uv_queue_work(uv_default_loop(), req, EIO_Fetch,  (uv_after_work_cb)EIO_After_Fetch);	
    
    //uv_ref(uv_default_loop());
    res->Ref();
    
    return;
  }

  
 FBResult::FBResult (XSQLDA *asqldap, isc_stmt_handle *astmtp, Connection *conn) : FBEventEmitter ()
  {
    sqldap = asqldap;
    stmt = *astmtp;
    connection = conn;
   // conn->doref();
  }

  void FBResult::releaseStatement() 
  {
    if(stmt){
     long SQLCODE;
	   isc_dsql_free_statement(status,&stmt,DSQL_drop);
	   if (status[1]){
		    SQLCODE = isc_sqlcode(status);
		    if(SQLCODE!=-901) // print error message if error is not 'Invalid Statement Handle', wich is normal when connection closed before FBresult is freed
		      printf("Error in free statement %s, %d, %ld\n",ErrorMessage(status,err_message,sizeof(this->err_message)),(int) status[1],SQLCODE);
		   /*ThrowException(Exception::Error(
		              String::Concat(FB_MAYBE_NEED_ISOLATE String::New("In FBResult::~FBResult, isc_dsql_free_statement - "),ERR_MSG(this, FBStatement))));
		   */           
	   }
	   else stmt = NULL;
	   
   }
  }

  void FBResult::finished() {
    releaseStatement();
  }
  
 FBResult::~FBResult()
  {
   
	//printf("Statement freed\n"); 
   if(sqldap) {
     FBResult::clean_sqlda(sqldap);
     free(sqldap);
     sqldap = NULL;
   }
   releaseStatement(); 
   
   
   
	   
  // printf("fbresult destructor !\n");
  // connection->dounref();
  }
  
