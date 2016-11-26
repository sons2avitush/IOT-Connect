var mysql = require('mysql');

var dbConnection = mysql.createConnection({
  host     : 'localhost',
  user     : 'root',
  password : 'bhushans',
  database : 'IOT Connect'
});

dbConnection.connect(function(err) {
  console.log("Connected to db");
  dbConnection.query('CREATE TABLE USERS(\
					SNO int(11) NOT NULL AUTO_INCREMENT PRIMARY KEY,\
					username varchar(20) NOT NULL UNIQUE CHECK (LENGTH(username) > 6),\
					password varchar(20) NOT NULL CHECK (LENGTH(password) > 6),\
					email varchar(50) NOT NULL,\
					Address1 varchar(200),\
					Address2 varchar(200),\
					City varchar(50),\
					Zip int(11))', function(err) 
									{
										if (err) throw err;
										console.log("Table 'users' is created");
									});
});