/*!
 *  Copyright by Denys Khanzhiyev aka xdenser
 *
 *  See license text in LICENSE file
 * 
 * 
 * 
 */


/*
 * 
 *  Utility functions to map API types to buffer calls
 * 
 */

function ctype(getFunc,setFunc,size)
{
    this.get = getFunc;
    this.set = setFunc;
    this.size = size;
}

function returnChunk(buf,offset,length)
{
    return buf.slice(offset,offset+this.size*length);
}

function EnotImplemented(){
        throw new Error('not implemented');
}

var
 POINTER_SIZE = (function(){
     switch(process.arch){
        case 'arm': return 4; // not sure in fact
        case 'ia32': return 4;
        case 'x64': return 8;
        default: return 4;
     }
 })(),
 C_TYPE = {
     
    'short': new ctype(function(buf,offset){
         return buf.readInt16LE(offset);
    },function(buf,offset,val){
         buf.writeInt16LE(val,offset);
    },2),
    
    'int': new ctype(function(buf,offset){
         return buf.readInt32LE(offset);
    },function(buf,offset,val){
         buf.writeInt32LE(val,offset);
    },4),
    
    'char[]': new ctype( returnChunk
     ,EnotImplemented,1),
    
    'void': new ctype(function(buf,offset,length){
        var l = length?offset+length:null;
        return buf.slice(offset,l);
    },EnotImplemented,1),
    
    'void*': new ctype(returnChunk,EnotImplemented,POINTER_SIZE),
    
    'char*': new ctype(returnChunk,EnotImplemented,POINTER_SIZE)
    
     
 },ISC_TYPE ={
    ISC_LONG: C_TYPE['int']     
 },SQLDA_DESC; 
 
  


!function(){
    
   var p = 0;
   function field(type,length){
       var ts = length||1,
            s = type.size*(ts);
       return {
           type: type,
           offset: (p+=s,p-s),
           length: ts,
           byteLength: s 
       };
   }
   
   XSQLDA_DESC = {
     version: field(C_TYPE['short']),
     sqldaid: field(C_TYPE['char[]'],8),
     sqldabc: field(ISC_TYPE.ISC_LONG),
        sqln: field(C_TYPE['short']),
        sqld: field(C_TYPE['short']),
      sqlvar: field(C_TYPE['void'])
   };
   
   console.log(XSQLDA_DESC);
   
   p = 0;
   XSQLVAR_DESC = {
         sqltype: field(C_TYPE['short']),
        sqlscale: field(C_TYPE['short']),
      sqlsubtype: field(C_TYPE['short']),
          sqllen: field(C_TYPE['short']),
         sqldata: field(C_TYPE['char*']),
          sqlind: field(C_TYPE['void*']),
  sqlname_length: field(C_TYPE['short']),
         sqlname: field(C_TYPE['char[]'],32),
  relname_length: field(C_TYPE['short']),
         relname: field(C_TYPE['char[]'],32),
  ownname_length: field(C_TYPE['short']),
         ownname: field(C_TYPE['char[]'],32), 
aliasname_length: field(C_TYPE['short']),
       aliasname: field(C_TYPE['char[]'],32),
     STRUCT_SIZE: p  
  };
    
}();


function getFieldVal(buf,field)
{
  return field.type.get(buf,field.offset);
}

function XSQLDA(buf)
{
    this._buffer = buf;
}


XSQLDA.prototype.getVersion = function(){
   return getFieldVal(this._buffer,XSQLDA_DESC.version);
}

XSQLDA.prototype.getSqlN = function(){
   return getFieldVal(this._buffer,XSQLDA_DESC.sqln);
}

XSQLDA.prototype.getSqlD = function(){
   return getFieldVal(this._buffer,XSQLDA_DESC.sqld);
}

XSQLDA.prototype.getSQLVAR = function(index)
{
    var o = XSQLDA_DESC.sqlvar.offset+XSQLVAR_DESC.STRUCT_SIZE * index;
    b = this._buffer.slice(o,o+XSQLVAR_DESC.STRUCT_SIZE);
    return new XSQLVAR(b); 
}


function XSQLVAR(buf)
{
   this._buffer = buf; 
}

XSQLVAR.prototype.getSqlType = function(){
   return getFieldVal(this._buffer,XSQLVAR_DESC.sqltype); 
}

XSQLVAR.prototype.getSqlScale = function(){
   return getFieldVal(this._buffer,XSQLVAR_DESC.sqlscale); 
}

XSQLVAR.prototype.getSqlSubType = function(){
   return getFieldVal(this._buffer,XSQLVAR_DESC.sqlsubtype); 
}

XSQLVAR.prototype.getSqlLen = function(){
   return getFieldVal(this._buffer,XSQLVAR_DESC.sqllen); 
}

XSQLVAR.prototype.getSqlData = function(){
   return getFieldVal(this._buffer,XSQLVAR_DESC.sqldata); 
}

XSQLVAR.prototype.getSqlInd = function(){
   return getFieldVal(this._buffer,XSQLVAR_DESC.sqlind); 
}

XSQLVAR.prototype.getSqlNameLength = function(){
   return getFieldVal(this._buffer,XSQLVAR_DESC.sqlname_length); 
}

XSQLVAR.prototype.getSqlName = function(){
   return getFieldVal(this._buffer,XSQLVAR_DESC.sqlname); 
}


XSQLVAR.prototype.getRelNameLength = function(){
   return getFieldVal(this._buffer,XSQLVAR_DESC.relname_length); 
}

XSQLVAR.prototype.getRelName = function(){
   return getFieldVal(this._buffer,XSQLVAR_DESC.relname); 
}

XSQLVAR.prototype.getOwnNameLength = function(){
   return getFieldVal(this._buffer,XSQLVAR_DESC.ownname_length); 
}

XSQLVAR.prototype.getOwnName = function(){
   return getFieldVal(this._buffer,XSQLVAR_DESC.ownname); 
}

XSQLVAR.prototype.getAliasNameLength = function(){
   return getFieldVal(this._buffer,XSQLVAR_DESC.aliasname_length); 
}

XSQLVAR.prototype.getAliasName = function(){
   return getFieldVal(this._buffer,XSQLVAR_DESC.aliasname); 
}

exports.XSQLDA = XSQLDA;
exports.XSQLVAR = XSQLVAR;

