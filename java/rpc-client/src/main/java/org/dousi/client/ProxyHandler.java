package org.dousi.client;

import java.lang.reflect.InvocationHandler;
import java.lang.reflect.Method;
import java.util.concurrent.CompletableFuture;


public class ProxyHandler<T> implements InvocationHandler {

    private NettyRpcClient nettyRpcClient;

    public ProxyHandler(NettyRpcClient nettyRpcClient, Class<T> clazz) {
        this.nettyRpcClient = nettyRpcClient;
    }

    @Override
    public Object invoke(Object proxy, Method method, Object[] args) throws Throwable {
        CompletableFuture<Object> future = nettyRpcClient.submit(method.getReturnType(), method.getName(), args);
        return future.get();
    }

}
