/*
Copyright by Denys Khanzhiyev
See license text in LICENSE file
*/

// Load configuration
var cfg = require("../config").cfg;

// Require modules
var
  fb_binding = require("../../build/default/binding");
 

exports.AsyncConnection = function (test) {
  test.expect(4);
  var conn = new fb_binding.Connection;
  conn.connect(cfg.db, cfg.user, cfg.password, cfg.role, function(){
     test.ok(conn.connected,"Connected to database");
     conn.query("select * from rdb$relations", function(err,res){
         test.ok(!err,"No Error");   
         test.ok(res,"Can query");   
         conn.disconnect();
         test.ok(!conn.connected,"Disconnected to database");
         test.done();
     });
  });
};

exports.AsyncQueryWithError = function (test) {
  test.expect(4);
  var conn = new fb_binding.Connection;
  conn.connect(cfg.db, cfg.user, cfg.password, cfg.role, function(){
      test.ok(conn.connected,"Connected to database");
      conn.query("select * from non_existent_table", function(err,res){
         test.ok(err,"There is error");   
         test.ok(!res,"No result");   
         conn.disconnect();
         test.ok(!conn.connected,"Disconnected to database");
         test.done();
      });
  });
}

exports.AsyncFetch = function(test){
  test.expect(6);
  var conn = new fb_binding.Connection;
  conn.connectSync(cfg.db, cfg.user, cfg.password, cfg.role);
  test.ok(conn.connected,"Connected to database");
  var res = conn.querySync("select * from rdb$relations");
  test.ok(res,"There is result");
  res.fetch(1,true, function(err,row){
    test.ok(!err, "No error!");
    test.ok(row,"There is row");
    test.ok(row instanceof Object,"is object"); 
    test.ok(!(row instanceof Array),"not array"); 
    conn.disconnect();
    test.done();
  });
  
} 



