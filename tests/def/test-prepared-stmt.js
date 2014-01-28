/*
Copyright by Denys Khanzhiyev
See license text in LICENSE file
*/


// Load configuration
var cfg = require("../config").cfg;
var sys = require("util");

// Require modules
var
  fb_binding = require("../../firebird");
var 
  testCase = require("nodeunit").testCase;  


module.exports = testCase({
         setUp: function(callback){
           this.conn = new fb_binding.createConnection();
           this.conn.connectSync(cfg.db, cfg.user, cfg.password, cfg.role);
           this.conn.querySync('create table PREPARED_TEST_TABLE ( test_field1 INT, test_field2 VARCHAR(10))');
           var sql =  'create or alter procedure TEST_PROC(\n'+
               '"INPUT" varchar(25))\n'+
               'returns (\n'+
               '"OUTPUT" varchar(25))\n'+
               'as\n'+
               'BEGIN\n'+
               '  output = input;\n'+
               '  suspend;\n'+
               'END\n';
           this.conn.querySync(sql);
           callback();
         },
         tearDown: function(callback){
           this.conn.disconnect();
           this.conn = null;
           var conn = new fb_binding.createConnection();
           conn.connectSync(cfg.db, cfg.user, cfg.password, cfg.role);
           conn.querySync('drop table PREPARED_TEST_TABLE');    
           conn.querySync('drop procedure TEST_PROC');    
           conn.disconnect();
           callback();
         },
         commitAll: function(test){
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
         },
         commitEvery:function(test){
            var data = [
            [1,'string 1'],
            [2,'string 2'],
            [3,'string 3'],
            [4,null]
           ];
           
           test.expect(3+data.length*2);
           var stmt = this.conn.prepareSync('insert into PREPARED_TEST_TABLE (test_field1, test_field2) values (?,?)');
           test.ok(stmt, 'statement returned');
           var i;
           for(i=0;i<data.length;i++)
           {
            if(!this.conn.inTransaction) this.conn.startTransactionSync();
            stmt.execSync.apply(stmt,data[i]);
            this.conn.commitSync();
           }; 
           

           var stmt = this.conn.prepareSync('select test_field2 from PREPARED_TEST_TABLE where test_field1=?');
           
           test.ok(stmt, 'statement returned');
           
           if(!this.conn.inTransaction) this.conn.startTransactionSync();
           test.ok(this.conn.inTransaction, 'inTransaction');
           for(i=0;i<data.length;i++)
           {
            stmt.execSync(data[i][0]);
            var row = stmt.fetchSync("all",true);
            test.ok(row, 'row returned');
            //console.log(sys.inspect(row));
            test.equal(data[i][1],row[0].TEST_FIELD2,'test_field2 equal');
           }
           test.done();
             
         },
         badArgNum:function(test){
           test.expect(2);
           var stmt = this.conn.prepareSync('insert into PREPARED_TEST_TABLE (test_field1, test_field2) values (?,?)');
           test.ok(stmt, 'statement returned');
           test.throws(function(){
             stmt.execSync(0);
           },'wrong number of arguments');
           test.done();
         },
         asyncExec:function(test){
           test.expect(3);
           var stmt = this.conn.prepareSync('insert into PREPARED_TEST_TABLE (test_field1, test_field2) values (?,?)');
           var data = [1,'boo'];
           stmt.exec(data[0],data[1]);
           var self = this;
           stmt.once('result',function(err){
                test.ok(!err,'no error on insert');
        	var stmt1 = self.conn.prepareSync('select test_field2 from PREPARED_TEST_TABLE');                          
        	stmt1.once('result',function(err){
        	  test.ok(!err,'no error on select');
        	  var row = stmt1.fetchSync("all",false); 
        	  test.equal(data[1],row[0][0],'returned data equal');
        	  test.done();
        	});
        	stmt1.exec();
           });
           
         },
         execProcSync:function(test){
            test.expect(2);
            var stmt = this.conn.prepareSync('execute procedure TEST_PROC(?)');
            var data = 'test';
            var res = stmt.execSync(data);
            test.ok(res,'have result');
            test.equal(res.OUTPUT,data,'returned data equal'); 
            test.done();
         },
         execProcAsync:function(test){
            test.expect(2);
            var stmt = this.conn.prepareSync('execute procedure TEST_PROC(?)');
            var data = 'testAsync';
            stmt.exec(data);
            stmt.once('result',function(err,res){
		test.ok(res,'have result');
        	test.equal(res.OUTPUT,data,'returned data equal'); 
        	test.done();               
            });
         }          
         
});