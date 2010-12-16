/*
Copyright by Denys Khanzhiyev
See license text in LICENSE file
*/

// Load configuration
var cfg = require("../config").cfg;

// Require modules
var
  fb_binding = require("../../build/default/binding");

exports.Connect = function (test) {
  test.expect(1);
  var conn = new fb_binding.Connection;
  test.ok(conn, "Connection inited");
  test.done();
};

