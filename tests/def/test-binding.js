/*
Copyright by Denys Khanzhiyev
See license text in LICENSE file
*/

// Load configuration
var cfg = require("../config").cfg;

// Require modules
var
  fb_binding = require("../../build/default/binding");

exports.ConnectionBinding = function (test) {
  test.expect(11);
  test.ok("Connection" in fb_binding, "Connection");
  var conn = new fb_binding.Connection;
  test.ok(conn, "Connection created");
  test.ok("connectSync" in conn, "connectSync");
  test.ok("connect" in conn, "connect");
  test.ok("connected" in conn, "connected");
  test.ok("disconnect" in conn, "disconnect");
  test.ok("querySync" in conn, "querySync");
  test.ok("query" in conn, "query");
  test.ok("query" in conn, "query");
  test.ok("addFBevent" in conn,"addFBevent");
  test.ok("deleteFBevent" in conn,"deleteFBevent");
  test.done();
};

exports.FBResultBinding = function(test){
 test.expect(3);
 var conn = new fb_binding.Connection; 
 conn.connectSync(cfg.db, cfg.user, cfg.password, cfg.role); 
 test.ok(conn.connected,"Connected to database");
 var res = conn.querySync("select * from rdb$database");
 test.ok("fetchSync" in res, "fetchSync");
 test.ok("fetch" in res, "fetch");
 conn.disconnect();
 test.done();
}


