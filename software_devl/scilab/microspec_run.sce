
dev_path = "/dev/ttyUSB0";

hserial = openserial(dev_path,"115200,n,8,1")

p1 = scf();
xtitle("sin(t) versus t");



while(1)

    writeserial(hserial,"start single!"+ascii(13));
    sleep(100);
//    serialstatus(hserial);
//    sleep(100);
    recv_str=readserial(hserial);

    A = asciimat(recv_str);
    A = A(1:600);
    // select every 2nd and shift left
    A0 = A(2:2:$) * (2^8);
    A1 = A(1:2:$);
    B = A0 + A1;

    clf(p1,'clear');
    x = [1:length(B)];
    plot2d(x,B,rect=[0,0,310,50000]);

    sleep(500);
end

