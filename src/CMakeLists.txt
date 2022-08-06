# 把当前路径下的所有源文件名字放入变量名SRC_LIST里面
aux_source_directory(. SRC_LIST)

# 表示生成可执行文件ChatServer，由SRC_LIST变量所定义的源文件
add_executable(mysqlserver ${SRC_LIST})

# 指定可执行文件链接时需要依赖的库文件
target_link_libraries(mysqlserver mysqlclient pthread)