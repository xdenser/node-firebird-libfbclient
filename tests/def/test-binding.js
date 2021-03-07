/*
Copyright by Denys Khanzhiyev
See license text in LICENSE file
*/

// Load configuration
var cfg = require("../config").cfg;

// Require modules
var
  safe_con   = require("../../firebird.js"),
  fb_binding = safe_con.binding;

exports.ConnectionBinding = function (test) {
  test.expect(23);
  test.ok("Connection" in fb_binding, "Connection");
  var conn = new fb_binding.Connection;
  test.ok(conn, "Connection created");
  test.ok("connectSync" in conn, "connectSync");
  test.ok("connect" in conn, "connect");
  test.ok("connected" in conn, "connected");
  test.ok("disconnect" in conn, "disconnect");
  test.ok("querySync" in conn, "querySync");
  test.ok("query" in conn, "query");
  test.ok("addFBevent" in conn,"addFBevent");
  test.ok("deleteFBevent" in conn,"deleteFBevent");
  test.ok("commit" in conn, "commit");
  test.ok("commitSync" in conn, "commitSync");
  test.ok("inTransaction" in conn, "inTransaction");
  test.ok("rollback" in conn, "rollback");
  test.ok("rollbackSync" in conn, "rollbackSync");
  test.ok("prepareSync" in conn, "prepareSync");
  test.ok("inAsyncCall" in conn, "inAsyncCall");
  test.ok("startTransactionSync" in conn, "startTransactionSync");
  test.ok("newBlobSync" in conn, "newBlobSync");
  test.ok("startNewTransactionSync" in conn, "startNewTransactionSync");
  test.ok("startNewTransaction" in conn, "startNewTransaction");
  test.ok("startTransactionSync" in conn, "startTransactionSync");
  test.ok("startTransaction" in conn, "startTransaction");
  test.done();
};

exports.SafeConnectionBinding = function (test) {
  test.expect(22);
  test.ok("createConnection" in safe_con, "createConnection");
  var conn = safe_con.createConnection();
  test.ok(conn, "Connection created");
  test.ok("connectSync" in conn, "connectSync");
  test.ok("connect" in conn, "connect");
  test.ok("connected" in conn, "connected");
  test.ok("disconnect" in conn, "disconnect");
  test.ok("querySync" in conn, "querySync");
  test.ok("query" in conn, "query");
  test.ok("addFBevent" in conn,"addFBevent");
  test.ok("deleteFBevent" in conn,"deleteFBevent");
  test.ok("commit" in conn, "commit");
  test.ok("commitSync" in conn, "commitSync");
  test.ok("inTransaction" in conn, "inTransaction");
  test.ok("rollback" in conn, "rollback");
  test.ok("rollbackSync" in conn, "rollbackSync");
  test.ok("inAsyncCall" in conn, "inAsyncCall");
  test.ok("prepareSync" in conn, "prepareSync");
  test.ok("newBlobSync" in conn, "newBlobSync");
  test.ok("startTransactionSync" in conn, "startTransactionSync");
  test.ok("startTransaction" in conn, "startTransaction");
  test.ok("startNewTransactionSync" in conn, "startNewTransactionSync");
  test.ok("startNewTransaction" in conn, "startNewTransaction");
  test.done();
};

exports.FBResultBinding = function(test){
 test.expect(4);
 var conn = new fb_binding.Connection; 
 conn.connectSync(cfg.db, cfg.user, cfg.password, cfg.role); 
 test.ok(conn.connected,"Connected to database");
 var res = conn.querySync("select * from rdb$database");
 test.ok("fetchSync" in res, "fetchSync");
 test.ok("fetch" in res, "fetch");
 test.ok("inAsyncCall" in res, "inAsyncCall");
 conn.disconnect();
 test.done();
}

exports.FBblobBinding = function(test){
 test.expect(6);
 var blob = fb_binding.FBblob.prototype;
 test.ok("_readSync" in blob, 'readSync');
 test.ok("_read" in blob, 'read');
 test.ok("_openSync" in blob, '_openSync');
 test.ok("_closeSync" in blob, '_closeSync');
 test.ok("_writeSync" in blob, '_writeSync');
 test.ok("_write" in blob, '_write');
 test.done();
}

exports.FBstatementBinding = function(test){
 test.expect(4);
 var stmt = fb_binding.FBStatement.prototype;
 test.ok("execSync" in stmt, 'execSync');
 test.ok("exec" in stmt, 'exec');
 test.ok("execInTransSync" in stmt, 'execInTransSync');
 test.ok("execInTrans" in stmt, 'execInTrans');
// test.ok("inAsyncCall" in stmt, "inAsyncCall");
 test.done();
}

exports.TransactionBinding = function (test) {
  test.expect(11);
  var trans = fb_binding.Transaction.prototype;
  test.ok("start" in trans, "commit");
  test.ok("startSync" in trans, "startSync");
  test.ok("commit" in trans, "commit");
  test.ok("commitSync" in trans, "commitSync");
  test.ok("rollback" in trans, "rollback");
  test.ok("rollbackSync" in trans, "rollbackSync");
  test.ok("querySync" in trans, "querySync");
  test.ok("query" in trans, "query");
  test.ok("prepareSync" in trans, "prepareSync");
  var conn = new fb_binding.Connection;
  conn.connectSync(cfg.db, cfg.user, cfg.password, cfg.role);
  var transInsts = conn.startNewTransactionSync();
  test.ok("inTransaction" in transInsts, "inTransaction");
  test.ok("inAsyncCall" in transInsts, "inAsyncCall");
  test.done();
}

exports.Lc_ctype = function(test) {
  test.expect(3);
  var conn = safe_con.createConnection({
    lc_ctype  : "ASCII",
    lc_ctype_decode: function (buffer) {
      // test buffer
      test.ok(buffer instanceof Buffer, "buffer expected");
      return buffer;
    }
  });
  conn.connectSync(cfg.db, cfg.user, cfg.password, cfg.role);
  var res = conn.querySync("select first 1 RDB$RELATION_NAME from rdb$relations").fetchSync("all",true);
  test.ok(res,"Query");
  test.ok(res[0]["RDB$RELATION_NAME"] instanceof Buffer,"Buffer returned"); 
  test.done();
}

