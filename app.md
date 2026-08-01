// 使用注册中间件?

// 每次请求都会从
const app = new App(port)
走一次这个链路
app.run();

App.use(ParserHttp)
App.use(authorize)

// Router1 -> 等于 /list get
// Router2 -> 等于 /edit put
// Router3 -> 等于 /add post
App.use(Router1)
App.use(Router2)

问题在于 router1、.. n 使用 App.use 来注册吗? 还是使用 App.useGet(),App.usePost这些来?
