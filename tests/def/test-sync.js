/*
Copyright by Denys Khanzhiyev
See license text in LICENSE file
*/

// Load configuration
var cfg = require("../config").cfg;

// Require modules
var
  fb_binding = require("../../build/default/binding");
 

exports.SyncConnection = function (test) {
  test.expect(3);
  var conn = new fb_binding.Connection;
  conn.connectSync(cfg.db, cfg.user, cfg.password, cfg.role);
  test.ok(conn.connected,"Connected to database");
  var res = conn.querySync("select * from rdb$relations");
  test.ok(res,"Can query");
  conn.disconnect();
  test.ok(!conn.connected,"Disconnected to database");
  test.done();
};



