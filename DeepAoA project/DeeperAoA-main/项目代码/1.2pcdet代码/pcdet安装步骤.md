# pcdet安装步骤

注意！：由于原作者的项目太大，我们没有上传到我们的github上。保存在了移动硬盘的deepAoA_largePrgram_backup



需要配置环境YX，具体这个环境是云翔学长配置的。需要注意的是要写清楚包的版本号。然后Pytorch1.4以上也可以不用docker安装稀疏卷积包，就正常安装就可以了。

安装好环境后，进入OpenPCDet/tools，就可以发现我们需要用到的demo.py和ori_demo.py了，整个项目我们也就改了这两个文件，ori_demo.py是原作者的demo.py。现在的demo.py就是获取真值整个流程中的一个步骤。

