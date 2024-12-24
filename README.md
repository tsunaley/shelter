### 编译
编译工具前请确保已安装一下依赖项：
```
sudo apt install -y pkg-config libssl-dev asciidoctor
```

#### 编译工具
执行以下命令，编译构建和运行项目所需的工具：

```
make tools
```

#### 编译项目
运行以下命令编译 Shelter 项目：

```
make
```
### 6. 使用方法
编译完成后，可以使用 Shelter 命令行工具来构建机密虚拟机并运行敏感程序。
#### Shelter 命令
```
./shelter help
```

### 7. 测试示例

#### 将敏感程序放入机密虚拟机中
将要放入敏感程序机密虚拟机中的敏感程序静态编译，可以使用`tests/ `目录下的`md5.c`测试。
静态编译：
```
cd tests/
gcc -static -o md5 md5.c
```

构建Shelter镜像并指定敏感程序，敏感程序会放入机密虚拟机的`/bin`目录下：
```
cd ../
sudo ./shelter build -t md5 -f tests/md5 -a "/usr/local/bin/attestation-agent"

```

客户端运行远程认证：
```
cd ra/
sh ra.sh
```

运行敏感程序：
```
cd ../
sudo ./shelter run -t md5 -c /bin/md5 -i <Your remote attestation IP address> -m <Memory size(2G)>
```

#### 将根文件系统放入虚拟机中
将要放入敏感程序机密虚拟机中的敏感程序静态编译，可以使用`tests/ `目录下的`Dockerfile`测试。
构建镜像运行python程序：
```
cd tests/
docker build -t my-ml-app .
```

运行docker容器：
```
docker run my-ml-app
```

导出rootfs：
```
sudo docker export <Container id> -o rootfs.tar
```

向shelter指定要使用的根文件系统：
```
cd ../
sudo ./shelter build -t ml -r tests/rootfs.tar
```

客户端运行远程认证：
```
cd ra/
sh ra.sh
```

运行根文件系统中的程序：
```
cd ../
sudo ./shelter run -t ml -c "/usr/local/bin/python /app/ml_test.py" -m <Memory size(4G)> -i <Your remote attestation IP address>
```
