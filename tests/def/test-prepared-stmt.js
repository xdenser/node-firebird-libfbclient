/*
Copyright by Denys Khanzhiyev
See license text in LICENSE file
*/


// Load configuration
var cfg = require("../config").cfg;

// Require modules
var
  fb_binding = require("../../firebird");
var 
  testCase = require("../../tools/nodeunit").testCase;  


module.exports = testCase({
         setUp: function(callback){
           this.conn = new fb_binding.createConnection();
           this.conn.connectSync(cfg.db, cfg.user, cfg.password, cfg.role);
           this.conn.querySync('create table PREPARED_TEST_TABLE ( test_field1 INT, test_field2 VARCHAR(10))');
           callback();
         },
         tearDown: function(callback){
           this.conn.disconnect();
           this.conn = null;
           var conn = new fb_binding.createConnection();
           conn.connectSync(cfg.db, cfg.user, cfg.password, cfg.role);
           //conn.querySync('drop table PREPARED_TEST_TABLE');    
           conn.disconnect();
           callback();
         },
         prepare: function(test){
           test.expect(1);
           var stmt = this.conn.prepareSync('insert into PREPARED_TEST_TABLE (test_field1, test_field2) values (?,?)');
           test.ok(stmt, 'statement returned');
           stmt.execSync(1,'bububu');
           stmt.execSync(2,'babababb');
           stmt.execSync(3,'bebebe');
           test.done();
         }
});