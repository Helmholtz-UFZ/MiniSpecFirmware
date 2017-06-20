
dev_path = "/dev/ttyUSB0";

hserial = openserial(dev_path,"115200,n,8,1")

//sens = 39; //  0x15B00039
sens = 40; // 0x15B00040

if(sens == 39)
    a0 = 3.175107897E+02;   
    b1 = 2.683434727E+00;
    b2 = -8.912972570E-04;
    b3 = -1.001126990E-05;
    b4 = 1.813212164E-08;
    b5 = -7.003857032E-12;
else
    a0 = 3.161753258E+02;
    b1 = 2.694674830E+00;
    b2 = -1.012573343E-03;
    b3 = -9.156342751E-06;
    b4 = 1.546117984E-08;
    b5 = -4.078555504E-12;
end

pix = [1:300];
wavelength = a0 + b1.*pix + b2.*pix.^2 + b3.*pix.^3 + b4.*pix.^4 + b5.*pix.^5;

p1 = scf();


//while(1)

    writeserial(hserial,"start single!"+ascii(13));
    sleep(100);
    serialstatus(hserial)
//    sleep(100);
    recv_str=readserial(hserial);

    A = asciimat(recv_str);
    A = A(1:600);
    // select every 2nd and shift left
    A0 = A(2:2:$) * (2^8);
    A1 = A(1:2:$);
    B = A0 + A1;
    B = B(2:$);
//    last_valid = B(1)
//    first_valid = (last_valid+1) - 288
//    length(B)
//    pixel_data = B(first_valid:last_valid)

//    clf(p1,'clear');
    x = [1:length(B)];
    plot2d(x,B,style=[2]);
    xlabel("wavelegth[nm]");
    ylabel("ADC count [-]");

    sleep(500);
//end
closeserial(hserial);
