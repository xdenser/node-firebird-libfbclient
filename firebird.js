var binding = require("./build/default/binding");
var sys = require("sys");


var Connection =  binding.Connection;
var FBResult   =  binding.FBResult;

function MakeSafe(obj,meth){
  var super = obj.prototype[meth];
  obj.prototype[meth] = function(){
    if(this.inAsyncCall){
       var self = this;
       var args = arguments;
       this.once("fbStopAsync",function(){
          self[meth].apply(self,args);  
       });      
    }
    else super.apply(this,arguments);
  }
}

MakeSafe(Connection,"query");
MakeSafe(Connection,"commit");
MakeSafe(Connection,"rollback");
MakeSafe(FBResult,"fetch");


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
  
  
  if(callback)
   { 
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
           self.emit('drain', chunk, len);
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
          callback(null, res, cPos);
         } 
     });
   }     
   
  
}


exports.createConnection = function () {
  var c = new Connection;
  return c;
};


exports.createConnectionPool = function(settings){
   
}
