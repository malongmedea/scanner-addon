# scanner-addon

这是一个node.js addon项目，通过twain协议调用扫描仪

## 开发记录

### 64位程序不能加载32位dll文件

   1. 重新安装node.js 32位版本。node.js 64位版本可以编译，但不能运行测试
   2. 重新安装electron 32位版本

        ```bash
        npm install --arch=ia32 electron --save-dev
        或者
        yarn add --arch=ia32 electron -D
        ```

   3. node-addon 插件重新编译32位版本

        ```bash
        node-gyp rebuild --target=4.0.0 --arch=ia32 --dist-url=https://atom.io/download/electron
        ```

        其中--target指的是electron版本号。--arch指的是平台，在这里设置为ia32（即32位）。--dist-url指编译头文件下载地址。
   4. 参考资料： <https://blog.csdn.net/cnhk1225/article/details/53884710>

### 运行测试

    查阅 .vscode/launch.json 文件运行测试
    test.js中的Uint8Array构造函数参数应为一个窗口句柄值【这是重点中的重点】
    提示：必须连接扫描仪或者安装了虚拟扫描仪，否则无法运行。

## 采坑参考：

     1.《vsCode开发node.js addon 采坑记》<https://blog.csdn.net/love3d/article/details/87926558>