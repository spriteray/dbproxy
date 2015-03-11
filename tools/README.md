## Introduction

SQLBind是MySQL的表结构和C++对象的一个转换脚本。

* 将MySQL的表结构转换成C++对象;
* 将C++对象的修改自动拼接成SQL语句

## Usage

修改sqlbind.py脚本中的数据库配置

	" HOST - MySQL的服务器地址 "
	" USERNAME - 访问数据库的用户名 "
	" PASSWORD - 访问数据库的密码 "
	" DATABASE - 需要访问的数据库 "
	" PORT - MySQL的端口号 "

修改正确后

	" >python sqlbind.py <C++代码目录>"
