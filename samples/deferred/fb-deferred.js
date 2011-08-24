var Deferred = require('../../tools/node-deferred/lib/deferred.js').Deferred,
    fb=require('../../firebird');


exports.createConnection = fb.createConnection;

var Connection  =  fb.binding.Connection,
    FBResult    =  fb.binding.FBResult,
    FBStatement =  fb.binding.FBStatement;



function MakeDeferred(obj,meth,cb_num){
  var super = obj.prototype[meth];
  obj.prototype[meth] = function(){

     var d = new Deferred();
     var args = [].slice.call(arguments);
     args[cb_num] = function(){
       var err = arguments[0];
       var args = [].slice.call(arguments,1);
       if(err) d.errback(err);
       else d.callback(args);
     }
     super.apply(this,args);
     return d;
  }
}

MakeDeferred(Connection,'connect',4);
MakeDeferred(Connection,'query',2);
MakeDeferred(Connection,'commit',0);
MakeDeferred(Connection,'rollback',0);
MakeDeferred(FBResult,'fetch',3);


var superExec = FBStatement.prototype.exec;
FBStatement.prototype.exec = function(){
   var d = new Deferred();
   this.once('error', function(err){
      d.errback(err);
   });
   var self = this;
   this.once('result', function(){
      d.callback(self);
   });
   superExec.apply(this,arguments);
   return d;
};


/*
var superConnect = Connection.prototype.connect;
Connection.prototype.connect = function(db,user,pass,role,cb){
   var d = new Deferred();
   superConnect.call(this,db,user,pass,role,function (err){
      if(err) d.errback(err);
      else d.callback();
   });
   return d;
};

var superQuery = Connection.prototype.query;
Connection.prototype.query = function(sql,cb){
  var d = new Deferred();
  superQuery.call(this,sql,function(err,res){
      if(err) obj.emit('error',err);
      else obj.emit('result',res);
      if(cb) cb(err,res);
  });
};
*/



