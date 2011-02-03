var binding = require("./build/default/binding");
var sys = require("sys");


var Connection =  binding.Connection;


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

exports.createConnection = function () {
  var c = new Connection;
  return c;
};


exports.createConnectionPool = function(settings){
   
}
