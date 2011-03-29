/*
Copyright by Denys Khanzhiyev
See license text in LICENSE file
*/


// Load configuration
var cfg = require("../config").cfg;
var sys = require("sys");

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
           conn.querySync('drop table PREPARED_TEST_TABLE');    
           conn.disconnect();
           callback();
         },
         prepare: function(test){
           var data = [
            [1,'string 1'],
            [2,'string 2'],
            [3,'string 3'],
            [4,null]
           ];

           test.expect(2+data.length*2);
           var stmt = this.conn.prepareSync('insert into PREPARED_TEST_TABLE (test_field1, test_field2) values (?,?)');
           test.ok(stmt, 'statement returned');
           var i;
           for(i=0;i<data.length;i++)
           {
            stmt.execSync.apply(stmt,data[i]);
           }
           this.conn.commitSync();
           var res = this.conn.querySync('select * from PREPARED_TEST_TABLE');
           var rows = res.fetchSync('all',false);
           test.equal(data.length,rows.length,'number of records');
           for(i=0;i<data.length;i++)
           {
            test.equal(data[i][0],rows[i][0],'first field');
            test.equal(data[i][1],rows[i][1],'second field');
           }
           test.done();
         }
});