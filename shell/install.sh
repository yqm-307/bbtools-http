function generate_head_dir_and_copy_to_dstdir()
{
    dstdir=$1
    cp -rf src src_h
    find src_h -name '*.c' -o -name '*.cc' -print | xargs rm

    if [ ! -d "$dstdir/bbt" ];then
        sudo mkdir $dstdir/bbt
    fi

    if [ -d "$dstdir/bbt/http" ]
    then
        sudo rm -rf $dstdir/bbt/http
    fi

    sudo mv src_h $dstdir/bbt/http
    # info_build "copy over! cpp head file copy to ${dstdir}/bbt"
    rm -rf src_h
}

cd ..
generate_head_dir_and_copy_to_dstdir /usr/local/include

if [ ! -d "build" ];then
    mkdir build
fi

cd build
cmake ..
make 
sudo cp -rf lib/libybbt_http.so /usr/local/lib/
echo "安装完成"