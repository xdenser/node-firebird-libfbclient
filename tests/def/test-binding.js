/*
Copyright by Denys Khanzhiyev
See license text in LICENSE file
*/

// Load configuration
var cfg = require("../config").cfg;

// Require modules
var
  fb_binding = require("../../build/default/binding");

exports.Binding = function (test) {
  test.expect(5);
  test.ok("Connection" in fb_binding, "Connection");
  var conn = new fb_binding.Connection;
  test.ok(conn, "Connection created");
  test.ok("connectSync" in conn, "connectSync");
  test.ok("connected" in conn, "connected");
  test.ok("disconnect" in conn, "disconnect");
  test.done();
};


