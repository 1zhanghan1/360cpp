# ThreadPool



## PlainThreadPool

**创建接口**

`ThreadPool(int minThreads, int maxThreads = 0, int maxQueueLen = 500, int busyThreshold = 0, int freeThreshold = 0, InitType it = InitType::HUNGER, TaskQueueType tt = TaskQueueType::BLOCK_RINGBUFFER, FullOperate fo = FullOperate::REJECT);`

**参数解读：**

- `minThreads`：线程池最小线程数。取`max(1, minTHreads)`。如果maxThreads不是默认值，再取`min(minThreads, maxThreads)`。
- `maxThreads`：线程池最大线程数。
- `maxQueueLen`：任务队列长度。非必要。
- `busyThreshold`：忙碌判断标准，指任务队列堆积任务数量。
- `freeThreshold`：空闲判断标准，指阻塞线程数量。
- `it`：枚举类`InitType`的枚举值，标记初始化线程池的方式。
- `tt`：枚举类`TaskQueueType`的枚举值，标记任务队列的实现方式。
- `fo`：枚举类`FullOperate`的枚举值，标记任务队列满时的行为。



**枚举类常量**

`InitType`：线程池中线程数量达到`minThreads`的方式

- `HUNGER`：饥饿式初始化，线程池启动时直接申请`minThreads`个线程
- `LAZY`：懒惰式初始化，线程池中线程初始为0个，任务放进队列前如果阻塞数为0则申请一个线程资源，直至线程数达到`minTHreads`

`TaskQueueType`：任务队列的底层实现方式，以及线程安全的实现方式

- `BLOCK_QUEUE`：阻塞队列
- `BLOCK_RINGBUFFER`：阻塞环形缓冲
- `LOCKFREE_QUEUE`：无锁队列
- `LOCKFREE_RINGBUFFER`：无锁环形缓冲

`FullOperate`：任务队列满时的操作

- `REJECT`：拒绝，返回空future
- `EXCEPTION`：抛出异常



**动态调整**

线程池的线程数达到`minThreads`后，可以根据任务量进行线程数量的增减，动态调整的范围在`[minThreads, maxThreads]`之间。

默认关闭。通过设置`maxThreads`大于`minThreads`开启动态调整策略，将会开启守护线程，定期判断是否空闲和忙碌。

忙碌：阻塞的线程数为0， 且任务队列长度大于`busyThreshold`。

空闲：阻塞线程数大于`freeThreshold`。

如果满足开启动态调整的条件而没有指定`busyThreshold`和`freeThreshold`的值时，`busyThreshold`默认为任务队列最大长度的一半，`freeThreshold`默认为`minThreads`的一半。

### 使用示例

1. 一般使用
   无返回值

   ```c++
   #include <ThreadPool>
   
   void fun(int x){
   	//do something
   }
   
   int main(){
   	ThreadPool pool(4);
       pool.start();
       pool.submit(fun, 1);
       return 0;
   }
   
   ```

   有返回值
   ```c++
   #include <ThreadPool>
   
   int fun(int x){
   	//do something
       return x+1;
   }
   
   int main(){
   	ThreadPool pool(4);
       pool.start();
       auto res = pool.submit(fun, 1);
       int y = res.get();// 2
       return 0;
   }
   ```

   

2. 开启动态放缩
   ```c++
   // 线程数[4,5]，busyThreshold和freeThreshold取默认值
   ThreadPool pool1(4, 5);
   // 线程数[1,3]，任务队列长度为100，busyThreshold为3，freeThreshold为1
   ThreadPool pool2(1, 3, 100, 3, 1);
   ```

   

3. 启停控制
   支持中间`shutdown`然后在`start`

   ```C++
   ThreadPool pool(4);
   pool.start();
   //do something
   pool.shutdown();
   //do something
   pool.start();
   ```

   



## ComposeThreadPool

可以设置优先级别的线程池。一般场景下，额外提供一级优先级就够用了。所以只额外增加一个任务队列作为优先级较高的队列。优先级高的任务也并不是绝对会被最先执行，而是由0号线程单独执行此队列中的任务，其他线程执行其他任务。同样的0号线程在一般队列为空时不能被普通阻塞，而是超时阻塞。

**如果任务量很小的情况下，优先队列可能比一般队列要慢。**

**创建接口**

```c++
ComposeThreadPool(int maxUrgTask, int minThreads, int maxThreads = 0, int maxQueueLen = 500, int busyThreshold = 0,int freeThreshold = 0, InitType it = InitType::HUNGER, TaskQueueType tt = TaskQueueType::LOCKFREE_RINGBUFFER, FullOperate fo = FullOperate::REJECT);
```

**参数解读：**

- `maxUrgTask`：优先队列的最大长度，建议不要太大。
- 其余参数同`PlainThreadPool`

### 使用示例

```c++
// 优先队列长度为10，线程数为4
ComposeThreadPool pool(10, 4);
pool.start();
pool.urgSubmit(fun);
```





# 不同实现方式任务队列性能测试

使用PlainThreadPool，线程数4，不开启动态放缩，初始化模式采用HUNGER，队满策略采用REJECT，队列最大长度1100，任务数量1024。分别测试任务队列为阻塞队列、阻塞环形缓冲、无锁队列、无锁环形缓冲时执行时间。额外增加不使用线程池，四个线程完全并行的执行时间，数学计算得到理论运行时间，便于比较。

模拟四种场景：

1. 全是小任务，10ms。All Little
2. 全是大任务，100ms。All Big
3. 全是超大任务，500ms。All Large
4. 混合场景，一半是小任务，四分之一是大任务，四分之一是超大任务，随机submit顺序。Hybrid

submit场景分为单线程和多线程(4)。

单位毫秒。

|                    |               | All Litte | All Big | All Large | Hybrid  |
| :----------------: | :-----------: | :-------: | :-----: | :-------: | :-----: |
|   No ThreadPool    |       -       |   2560    |  25600  |  128000   |  39680  |
|     BlockQueue     | Single thread |  2595.54  | 25637.9 |  128039   | 39514.9 |
|                    | Multi thread  |  2793.21  | 25636.5 |  128037   | 39510.5 |
|  BlockRingBuffer   | Single thread |  2594.88  | 25636.4 |  128039   | 39514.5 |
|                    | Multi thread  |  2794.39  | 25636.5 |  128036   | 39650.2 |
|   LockfreeQueue    | Single thread |  2597.66  | 25638.6 |  128037   | 39496.6 |
|                    | Multi thread  |  2809.95  | 25636.3 |  128037   | 39717.2 |
| LockfreeRingBuffer | Single thread |  2595.29  | 25636.7 |  128036   | 39473.1 |
|                    | Multi thread  |  2835.97  | 25636.3 |  128036   |  39592  |

任务大小和提交次数选择的应该不太恰当，比较不出来什么，暂时先这样吧。。。。。。。。。
