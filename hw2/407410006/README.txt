OX

學號:407410006
姓名:鄧晴
環境:linux

執行:
1. make
2. ./server
3. ./client [IP] (此程式為0.0.0.0)

可用指令:
1. change user name: "1 aaa"
2. list:   "2"
3. invite: "3 aaa bbb"	(aaa invite bbb)
4. bbb會接收server訊息: "4 aaa invite bbb"	
5. 接受   : "5 y aaa" 
   不接受 : "5 n aaa" (在被邀請端輸入)
6. 接收server: "6" (game start)	

圈叉:
0 : 初始
1 : player1使用
2 : player2使用

遊戲時輸入九宮格位置(0~8)，為了與指令區分前面加上負號(-0~-8)

登出 : "logout"

可多個client登入
可列出所有使用者並請求同意
可順利遊戲至分出勝負
可登出
不提供註冊
