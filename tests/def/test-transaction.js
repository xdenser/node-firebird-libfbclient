/*
Copyright by Denys Khanzhiyev
See license text in LICENSE file
*/

// Load configuration
var cfg = require("../config").cfg;

// Require modules
var
  fb_binding = require("../../firebird.js");
var cfg = require("../config").cfg;


exports.setUp = function (callback) {
    this.conn = new fb_binding.createConnection();
    this.conn.connectSync(cfg.db, cfg.user, cfg.password, cfg.role);
    this.conn.querySync('create table TRANS_TEST_TABLE ( test_field1 INT, test_field2 VARCHAR(10))');
    callback();
}

exports.tearDown = function (callback) {
    this.conn.disconnect();
    this.conn = null;
    var conn = new fb_binding.createConnection();
    conn.connectSync(cfg.db, cfg.user, cfg.password, cfg.role);
    conn.querySync('drop table TRANS_TEST_TABLE');
    conn.disconnect();
    callback();
}

exports.testNewTransactionSync = function (test) {
    test.expect(2);
    try {
        var trans = this.conn.startNewTransactionSync();
        trans.querySync("insert into TRANS_TEST_TABLE (test_field1, test_field2) values (0,'some') ");
        var r1 = this.conn.querySync("select count(*) from TRANS_TEST_TABLE").fetchSync('all', false);
        test.equal(r1[0][0], 0, "Zero expected");
        trans.commitSync();
        this.conn.rollbackSync();
        var r2 = this.conn.querySync("select count(*) from TRANS_TEST_TABLE").fetchSync('all', false);
        test.equal(r2[0][0], 1, "One expected");
        test.done();
    } catch(e) {
        console.log("error", e);
    }
}

function wr(f) {
    return function () {
        try {
            f.apply(this, arguments);
        } catch (e) {
            console.log("Error in callback function ", f, e);
        };
    }
}

 exports.testNewTransactionAsync = function (test) {
        test.expect(4);
        var conn = this.conn;
        conn.startNewTransaction(wr(function (err, tr) {
            test.equal(err, null, "Error starting transaction");
            tr.querySync("insert into TRANS_TEST_TABLE (test_field1, test_field2) values (0,'some') ");
            var r1 = conn.querySync("select count(*) from TRANS_TEST_TABLE").fetchSync('all', false);
            test.equal(r1[0][0], 0, "Zero expected");
            tr.commit(wr(function (err) {
                test.equal(err, null, "Error commiting transaction");
                conn.rollbackSync();
                var r2 = conn.querySync("select count(*) from TRANS_TEST_TABLE").fetchSync('all', false);
                test.equal(r2[0][0], 1, "One expected");
                test.done();
            }));
        }));
 };

