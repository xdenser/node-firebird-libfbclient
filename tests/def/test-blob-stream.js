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
var 
  fs = require('fs');  


var util = {
            createTestTable: function(datatype){
              this.conn.querySync('create table BLOB_TEST_TABLE ( test_field '+datatype+')');
            },
            quote:function(val){ return "'"+val+"'";}
           };    


module.exports = testCase({
         setUp: function(callback){
           this.conn = new fb_binding.createConnection();
           this.conn.connectSync(cfg.db, cfg.user, cfg.password, cfg.role);
           callback();
         },
         tearDown: function(callback){
           this.conn.disconnect();
           this.conn = null;
           var conn = new fb_binding.createConnection();
           conn.connectSync(cfg.db, cfg.user, cfg.password, cfg.role);
           conn.querySync('drop table BLOB_TEST_TABLE');    
           conn.disconnect();
           callback();
         },
         fileInsert: function(test){
           test.expect(1);
           util.createTestTable.call(this,'BLOB SUB_TYPE 0');
           var stmt = this.conn.prepareSync('insert into BLOB_TEST_TABLE ( test_field ) values (?)');
           console.log(' before new blob stream created');
           strm = new fb_binding.Stream(this.conn.newBlobSync());
           console.log('after new blob stream created');
           test.ok(strm,'There is stream');
           var fstrm = fs.createReadStream('tests/testdata/image.gif');
           fstrm.pipe(strm);
           stmt.execSync(strm._blob);
           this.conn.commitSync();
           test.done();
         }
});
