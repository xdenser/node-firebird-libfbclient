var binding = require("./build/Release/binding");
var stream = require("stream");
var util = require("util");
var events = require('events');
var SchunkSize = 4*1024;


var Connection  =  binding.Connection;
var FBEventEmitter = binding.FBEventEmitter;
var FBResult    =  binding.FBResult;
var FBStatement =  binding.FBStatement;
var FBblob =  binding.FBblob;

// inherit FBEventEmitter (and all descendants ) from events.EventEmitter
FBEventEmitter.prototype.__proto__ = events.EventEmitter.prototype;


function MakeSafe(obj,meth){
  var superm = obj.prototype[meth];
  obj.prototype[meth] = function safe(){
    if(this.inAsyncCall){
       var self = this;
       var args = arguments;
       this.once("fbStopAsync",function(){
          safe.apply(self,args); 
          //superm.apply(self,args);  
       });      
    }
    else {
      superm.apply(this,arguments);  
    } 
  }
}

MakeSafe(Connection,"query");
MakeSafe(Connection,"commit");
MakeSafe(Connection,"rollback");
MakeSafe(FBResult,"fetch");
MakeSafe(FBStatement,"exec");
MakeSafe(FBblob,"_read");
MakeSafe(FBblob,"_write");

var superConnect = Connection.prototype.connect;
Connection.prototype.connect = function(db,user,pass,role,cb){
   var obj = this;
   superConnect.call(this,db,user,pass,role,function (err){
       if(err) obj.emit('error',err);
       else obj.emit('connected');
       if(cb) cb(err);
   });
};

var superQuery = Connection.prototype.query;
Connection.prototype.query = function(sql,cb){
  var obj = this;
  superQuery.call(this,sql,function(err,res){
      if(err) obj.emit('error',err);
      else obj.emit('result',res);
      if(cb) cb(err,res);
  });
};

binding.FBblob.prototype._readAll = function(initialSize, chunkSize, callback){
  if(initialSize instanceof Function)
   {
     callback = initialSize;
     chunkSize = null;
     initialSize = null;
   }
  else 
  if(chunkSize instanceof Function)
   {
     callback = chunkSize;
     chunkSize = null;
   } 
   
  if(!chunkSize) chunkSize = 1024;
  if(!initialSize) initialSize = 0;
  
  var chunk = new Buffer(chunkSize);   
  var res = new Buffer(initialSize);
  var cPos = 0;
  
  
  this._openSync();
  var self = this;
  this._read(chunk,function receiver(err,b,len){
         if(err)
         {
           self.emit('error',err);
         }
         else
         if(len>0)
         { 
           self.emit('data', chunk, len);
           if(res.length<=(cPos+len))
           {
             var nr = new Buffer(cPos+len);
             res.copy(nr);
             res = nr;
           }    
           chunk.copy(res,cPos,0,len);
           cPos = cPos + len;
           self._read(chunk,receiver);
         } 
         else 
         {
          self._closeSync();           
          self.emit('end',res,cPos);
          if(callback) callback(null, res, cPos);
         } 
 });
   
  
}


exports.createConnection = function () {
  var c = new Connection();
  return c;
};

exports.createConnectionPool = function(settings){
   
};

// Allows further extention
exports.binding = binding;

var buf = null;
function allocBuf(){
  buf = new Buffer(SchunkSize);   
}

function ReadStream(strm) {
  if(buf == null) allocBuf();
  
  strm._blob._read(buf,function s_rcv(err, b, len){
  
         if(err)
         {
           strm.emit('error',err);
         }
         else
         if(len>0)
         { 
           strm.emit('data', b, len);
           if(!strm._paused) strm._blob._read(buf,s_rcv);
         }
         else  
         {
           strm.emit('end');
         }
  });  
};

function Stream(blob){
 if(!(blob instanceof binding.FBblob )) {
    throw new Error('Expecting blob');
    //blob = new binding.FBblob();
 }     
 stream.Stream.call(this);
 this._blob = blob;
 this.readable = false;
 this.writeable = false;
 
 if(blob.isReadable)
 {
   this._blob._openSync();         
   this.readable = true;
   this._paused = true;
 } 
 else 
   this.writable = true;
};

util.inherits(Stream, stream.Stream);
exports.Stream = Stream;

Stream.prototype.pause = function(){
 this._paused = true;
};

Stream.prototype.resume = function(){
 this._paused = false;
 ReadStream(this);
};

Stream.prototype.destroy = function(){
 this._blob._closeSync();
 this.emit('close');
};

Stream.prototype.write = function(data, encoding, fd) {
  if (typeof data != 'string') {
  };
  var self = this;
  //this._blob._writeSync(data);
  this._blob._write(data,function(err){
     if(err) self.emit('error',err);
     //self.emit('drain');
  }); 
}

Stream.prototype.end = function(data, encoding, fd) {
  var self = this;
  if(data) this._blob._write(data,function(err){
     console.log('in blob write callback'); 
     if(err) self.emit('error',err);
     self.destroy();
  })
  else self.destroy();
   
}

