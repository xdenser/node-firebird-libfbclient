/*
Copyright by Denys Khanzhiyev
See license text in LICENSE file
*/

// Load configuration
var cfg = require("../config").cfg;

// Require modules
var
  fb_binding = require("../../build/default/binding");
 

exports.ASyncConnection = function (test) {
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

exports.ASyncQueryWithError = function (test) {
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



