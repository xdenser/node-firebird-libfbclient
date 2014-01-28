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


var util = {
            getDataTypeResult: function(datatype,value){
              this.conn.querySync('create table BLOB_TEST_TABLE ( test_field '+datatype+')');
              this.conn.querySync('insert into BLOB_TEST_TABLE (test_field) values ('+value+')');
              this.conn.commitSync();
              return this.conn.querySync('select first 1 test_field from BLOB_TEST_TABLE').fetchSync("all",false)[0][0];
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
         textBlob: function(test){
           test.expect(1);
           Data = 'some data in blob';
           var res = util.getDataTypeResult.call(this,'BLOB SUB_TYPE 1',util.quote(Data));
           var buf = new Buffer(1024);
           res._openSync();
           var len = res._readSync(buf);
           res._closeSync();
           test.equal(buf.toString('utf8',0,len),Data,'blob ');
           test.done();
         },
         asyncRead: function(test){
           test.expect(2);
           //Data = 'some data for async blob';
           chars = '0123456789abcdefgh';
           for(var i=0; i < 2000; i++) Data+=chars.charAt(Math.floor(Math.random()*chars.length));                

           var res = util.getDataTypeResult.call(this,'BLOB SUB_TYPE 1',util.quote(Data));
           var buf = new Buffer(2048);
           res._openSync();
           res._read(buf,function(err,b,len){
        	 test.ifError(err);
        	 
                 res._closeSync();
        	 test.equal(b.toString('utf8',0,len),Data,'blob ');
        	 test.done();	 
           });
         },
         readAll:function(test){
           test.expect(2);
           Data = '';
           chars = '0123456789abcdefgh';
           for(var i=0; i < 10000; i++) Data+=chars.charAt(Math.floor(Math.random()*chars.length));                
           var res = util.getDataTypeResult.call(this,'BLOB SUB_TYPE 1',util.quote(Data));
           res._readAll(0,function(err,res,len){
              test.ifError(err);
              test.equal(res.toString('utf8',0,len),Data,'equal Data');
              test.done();
           });
         }
}); 
