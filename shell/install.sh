
installpath="/usr/local/include"
libpath="/usr/local/lib"

cd ..
shell/bbt_copy_header_dir.sh $installpath bbt http

sudo cp build/lib/libbbt_http.so /usr/local/lib/

if [ ! -d "build" ];then
    mkdir build
fi

echo "安装完成"