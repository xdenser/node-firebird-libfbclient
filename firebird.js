var binding = require("./build/Release/binding");
var stream = require("stream");
var util = require("util");
var events = require('events');

var SchunkSize = 4*1024;


var Connection  =  binding.Connection;
var FBEventEmitter = binding.FBEventEmitter;
var FBResult    =  binding.FBResult;
var FBStatement =  binding.FBStatement;
var FBblob = binding.FBblob;
var Transaction = binding.Transaction;

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
MakeSafe(Connection, "rollback");
MakeSafe(Connection, "startTransaction");
//MakeSafe(Connection, "startNewTransaction");
MakeSafe(FBResult,"fetch");
MakeSafe(FBStatement, "exec");
MakeSafe(FBStatement, "execInTrans");
MakeSafe(FBblob,"_read");
MakeSafe(FBblob, "_write");
MakeSafe(Transaction, "start");
MakeSafe(Transaction, "query");
MakeSafe(Transaction, "commit");
MakeSafe(Transaction, "rollback");

var superConnect = Connection.prototype.connect;
Connection.prototype.connect = function(db,user,pass,role,cb){
   var obj = this;
   superConnect.call(this,db,user,pass,role,function (err){
       if(err && (!cb || obj.listeners('error').length)) obj.emit('error',err);
       else obj.emit('connected');
       if(cb) cb(err);
   });
};


var superQuery = Connection.prototype.query;
Connection.prototype.query = function(sql,cb){
  var obj = this;
  superQuery.call(this,sql,function(err,res){
      if(err && (!cb || obj.listeners('error').length)) obj.emit('error',err);
      else obj.emit('result',res);
      if(cb) cb(err,res);
  });
};

var superStartNewTransaction = Connection.prototype.startNewTransaction;
Connection.prototype.startNewTransaction = function (cb) {
    var obj = this;
    var tr = superStartNewTransaction.call(this, function (err) {
        if (err && (!cb || obj.listeners('error').length)) obj.emit('error', err);
        if (cb) cb(err, tr);
    });
}


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
  this._read(chunk, function receiver(err, b, isLast) {
         if(err)
         {
           self.emit('error',err);
         }
         else
         if(!isLast)
         { 
           self.emit('data', b);
           if(res.length<=(cPos+b.length))
           {
               var nr = new Buffer(cPos + b.length);
             res.copy(nr);
             res = nr;
           }    
           b.copy(res,cPos);
           cPos = cPos + b.length;
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


exports.createConnection = function (options) {
  var c = new Connection(options);
  return c;
};


// Allows further extension
exports.binding = binding;

function Stream(blob){


 if(!(blob instanceof binding.FBblob )) {
    throw new Error('Expecting blob');
    //blob = new binding.FBblob();
 }     
 stream.Stream.call(this);
 this._blob = blob;
 this._pendingWrites = 0;
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

function ReadStream(strm) {

    if (strm._buf == null) {
        strm._buf = new Buffer(SchunkSize);
    }

    strm._blob._read(strm._buf, function s_rcv(err, b, isLast) {

        if (err) {
            return strm.emit('error', err);
        }
        strm.emit('data', b);
        
        if (!strm._paused && !isLast) strm._blob._read(strm._buf, s_rcv);

        if (isLast) strm.emit('end');
    });
};

Stream.prototype.pause = function(){
 this._paused = true;
};

Stream.prototype.resume = function(){
 this._paused = false;
 ReadStream(this);
};

Stream.prototype.destroy = function () {
  this._destroyed = true;
  this.check_destroyed();
};

Stream.prototype.check_destroyed = function () {
    if (this._destroyed && !this._pendingWrites) {
        this._blob._closeSync();
        this.emit('close');
    }
}

Stream.prototype.write = function (data, encoding, fd) {
  if (this._destroyed) {
      throw new Error("Stream closed");
  }
  var self = this;
  this._pendingWrites++;
  this._blob._write(data, function (err) {
      self._pendingWrites--;
      if (err) self.emit('error', err);
      self.check_destroyed();
  }); 
}

Stream.prototype.end = function(data, encoding, fd) {
    var self = this;
    if (data) {
        this._pendingWrites++;
        this._blob._write(data, function (err) {
            self._pendingWrites--;
            if (err) self.emit('error', err);
            self.destroy();
        })
    }
    else this.destroy();
   
}

