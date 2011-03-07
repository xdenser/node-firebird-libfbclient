/*
Copyright by Denys Khanzhiyev
See license text in LICENSE file
*/

// Load configuration
var cfg = require("../config").cfg;

var zexports = {};
// Require modules
var
  fb_binding = require("../../firebird");


exports.TwoSmallQueries = function (test) {
  test.expect(6);
  var conn = fb_binding.createConnection();
  conn.connect(cfg.db, cfg.user, cfg.password, cfg.role, function(){
     test.ok(conn.connected,"Connected to database");
     var queries = 0;
     function check(){
       if(queries==2) 
       { 
        conn.disconnect();
        test.ok(!conn.connected,"Disconnected from database");
        test.done();
       }    
     };
     conn.query("select * from rdb$relations", function(err,res){
         test.ok(!err,"No Error");   
         test.ok(res,"Can query");   
         queries++;
         check();
     });
     conn.query("select * from rdb$relations", function(err,res){
         test.ok(!err,"No Error");   
         test.ok(res,"Can query");   
         queries++;
         check();
     });
  });
}

exports.AHundredOfQueries = function(test){
  var query_count = 200;
  test.expect(query_count*2+2);
  var conn = fb_binding.createConnection();
  conn.addListener('z',function(){});
  conn.setMaxListeners(0);
  
  var queried = 0;
  function Finish(){
    if(queried==query_count){
        conn.disconnect();
        test.ok(!conn.connected,"Disconnected from database");
        test.done();
    }
  }
  function MakeQuery(){
    conn.query("select * from rdb$relations", function(err,res){
         test.ok(!err,"No Error");   
         if(err) console.log(err.message);
         test.ok(res,"Can query");   
         queried++;
         Finish();
    });
  }
  conn.connect(cfg.db, cfg.user, cfg.password, cfg.role, function(){
     test.ok(conn.connected,"Connected to database");
     for(var i = 0; i< query_count; i++) MakeQuery();
  });     
}