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
  testCase = require("nodeunit").testCase;  
var 
  fs = require('fs');  
var 
  sys = require("util");


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
           test.expect(2);
           util.createTestTable.call(this,'BLOB SUB_TYPE 0');
           
           var conn = this.conn;
           function insertBlob(cb) {
               var stmt = conn.prepareSync('insert into BLOB_TEST_TABLE ( test_field ) values (?)');
               var blob = conn.newBlobSync();
               var strm = new fb_binding.Stream(blob);
               test.ok(strm, 'There is a stream');
               var fstrm = fs.createReadStream('tests/testdata/image.gif');
               fstrm.resume();
               fstrm.pipe(strm);

              
               strm.on("close", function () {
                   stmt.execSync(strm._blob);
                   conn.commitSync();
                   cb();
               });
               strm.on('error', function (err) {
                   console.log('error in write blob stream ',err);
               })
           }
           

           function checkBlob() {
               var res = conn.querySync("select * from BLOB_TEST_TABLE").fetchSync("all", false)[0][0];
               
               var strm = new fb_binding.Stream(res);
               var buffers = [];
               strm.on('data', function (data) {
                   buffers.push(data);
               })
               strm.on('end', function () {
                   var buf = Buffer.concat(buffers);
                   var expected = fs.readFileSync('tests/testdata/image.gif');
                   test.deepEqual(buf, expected, "Buffers not equal");
                   test.done();
               });
               
               strm.resume();
           }
           
           insertBlob(checkBlob);
           
         }
});
