var fb = require('./fb-deferred.js');

var conn = fb.createConnection();

conn.connect('test.fdb','sysdba','masterkey','')
 .trycatch()
 .addCallback(function(){
   console.log('connected');
   return conn.query("select * from rdb$relations");
 }) 
 .addCallback(function(res){ 
   console.log('fetching');
   return res.fetch("all",true,function(row){
     console.log(row);
   });
 })
 .addCallback(function(){
   console.log('all fetched');
 })
 .addErrback(function(err) {
   console.log('error');
   console.log(err);
 }); 