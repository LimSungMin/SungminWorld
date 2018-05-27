# SUNGMIN WORLD

## 프로젝트 설명 

Sungmin World 에 오신걸 환영합니다. 본 프로젝트는 게임회사 서버/네트워크 직군 구직용으로 사용할 개인 포트폴리오  입니다.

- 제작 기간 : 2018년 4월 4일 ~ 현재
- Client   : [Unreal Engine 4](https://github.com/LimSungMin/SungminWorld/tree/master/Source/SungminWorld)
- Server   : [C++ IOCP Server](https://github.com/LimSungMin/SungminWorld/tree/master/SungminWorld-Server)
- DataBase : MySQL
- 이메일   : lsmorigin@gmail.com
- 블로그   : https://lsm_origin.blog.me/

## 시연 영상

[![클릭시 이동](https://img.youtube.com/vi/H_Lc5JI-OZo/0.jpg)](https://www.youtube.com/watch?v=H_Lc5JI-OZo)
[(클릭시 이동)](https://www.youtube.com/watch?v=H_Lc5JI-OZo&t=28s)

## 서버 클래스 

|  class | description  |   
|---|---|
| [IocpBase](https://github.com/LimSungMin/SungminWorld/blob/master/SungminWorld-Server/IocpServer/IocpBase.h)  | IOCP 부모 클래스  |   
|  [MainIocp](https://github.com/LimSungMin/SungminWorld/blob/master/SungminWorld-Server/IocpServer/MainIocp.h) |  IocpBase를 상속받는 메인 서버 클래스 |   
| [DBConnector](https://github.com/LimSungMin/SungminWorld/blob/master/SungminWorld-Server/IocpServer/DBConnector.h)  | DB 쿼리 수행 클래스  |  

## 구현 기능

- [IOCP 실시간 서버](https://github.com/LimSungMin/SungminWorld/issues/17)
- [DB를 이용한 회원가입 & 로그인](https://github.com/LimSungMin/SungminWorld/issues/21)
- [채팅 기능](https://github.com/LimSungMin/SungminWorld/commit/69885a8cad211a0d48a3fc123f8129bf1323446d)
- [AI 몬스터](https://github.com/LimSungMin/SungminWorld/issues/10)