**并发线程模型**：并发线程能够读写共享的 heap 堆区。除此之外，每个线程持有局部变量的副本。Mosaic 不会自动在每条语句之后进行线程调度：我们需要手工插入 `sys_sched`。