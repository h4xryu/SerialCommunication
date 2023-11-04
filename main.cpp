
#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <unistd.h>
#include <iostream>
#include <string.h>
#include <ctype.h>
#include <vector>

int main(int argc, char **argv)
{
    const char *device = "/dev/ttyACM0";
    if (argv[1]) {
        device = argv[1];
    }

    int fd = open(device, O_RDWR | O_NOCTTY | O_NDELAY);
    //O_RDWR : 파일을 읽기/쓰기용으로 연다, O_NDELAY : non-blocking 입추력 옵션 읽고 쓸 데이터 없으면 -1 리턴
    // O_NOCTTY : ctrl + c 무시
    if (fd == -1) {
        fprintf(stderr, "open(%s): %s\n", device, strerror(errno));
        const char *device2 = "/dev/ttyACM0";
        fd = open(device2, O_RDWR | O_NOCTTY | O_NDELAY);
        if (fd == -1) {
            std::cout << "No connection" << std::endl;
            return 1;
        }
    }

    struct termios tty;

    if (tcgetattr(fd, &tty) != 0) //메모리 포인터 주소 확인해서 열려있는지 없는지 속성 값 반환
    {
        fprintf(stderr, "tcgetattr(%s): %s\n", device, strerror(errno));
        return 1;
    }


    //control mode flag

    tty.c_cflag &= ~PARENB; //패리티 없음
    tty.c_cflag &= ~CSTOPB; // 1개의 정지 비트
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8; //8비트
    tty.c_cflag &= ~CRTSCTS;
    tty.c_cflag |= CREAD | CLOCAL;

    tty.c_lflag &= ~ICANON;//incannical한 명령어 (\n단위 아님)
    tty.c_lflag &= ~ECHO; //에코설정
    tty.c_lflag &= ~ECHOE;
    tty.c_lflag &= ~ECHONL;
    tty.c_lflag &= ~ISIG;
    tty.c_iflag &= ~(IXON | IXOFF | IXANY); //시작비트 종료비트 미사용
    tty.c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL);

    tty.c_oflag &= ~OPOST;
    tty.c_oflag &= ~ONLCR;

    tty.c_cc[VTIME] = 0;
    tty.c_cc[VMIN] = 0;

    cfsetispeed(&tty, B115200);
    cfsetospeed(&tty, B115200);

    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        fprintf(stderr, "tcgetattr(%s): %s\n", device, strerror(errno));
        return 1;
    }

    // 아두이노로 데이터 전송

    std::string data;
    std::cout << "Enter data to send to Arduino (1(start) or 0(stop) or 2(Interrupt)): "; // Qt에서는 값을 입력하지 않고 직접 보낼거임
    std::cin >> data;

    if (data != "1" && data != "0" && data != "2") {
        std::cout << "Invalid input. Only 0,1,2 is allowed." << std::endl;
        close(fd);
        return 1;
    }



    int n = write(fd, data.c_str(), data.size());


    if (n < 0) {
        fprintf(stderr, "write(%s): %s\n", device, strerror(errno));
        close(fd);
        return 1;
    }

    unsigned char buffer[100];
    std::vector<double> dataBuff;
    std::string tmp;
    int tmp2 = 0;
    bool stx = false;
    bool isfull = false;
    bool breakPoint = false;

    while(1){
        ssize_t num_bytes = read(fd, &buffer, sizeof(buffer));

        if (num_bytes == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                continue;
            }
            fprintf(stderr, "read(%s): %s\n", device, strerror(errno));
            break;
        }

        if (num_bytes > 0) {
            printf("%zi bytes received:\n", num_bytes);

            printf(" (");
            for (ssize_t i = 0; i < num_bytes; ++i) {
                printf(" %c", isprint(buffer[i]) ? buffer[i] : '?');

                if (buffer[i] == '@'){
                    stx = true;
                    continue;
                }
                if (buffer[i] == '#' && stx){

                    std::cout << stoi(tmp);
                    dataBuff.push_back(stoi(tmp));
                    stx = false;
                    tmp.clear();
                    isfull = false;
                    continue;
                }
                if (stx && isprint(buffer[i])) {
                    unsigned char ttmp;
                    ttmp = buffer[i];
                    tmp += ttmp;

                    isfull = true;
                    continue;
                }

            }

            printf(")\n");
            if(breakPoint){
                break;
            }
        }
    }

    close(fd);

    return 0;
}
/*
#include <iostream>
#include <fstream>
#include <string>

int main() {
    const char* serialPort = "/dev/ttyACM1"; // 아두이노와 연결된 시리얼 포트 경로





    std::ofstream serialOut(serialPort); // 시리얼 포트를 쓰기 모드로 열기

    if (!serialOut) {
        std::cerr << "Failed to open the serial port." << std::endl;
        return 1;
    }

    std::ifstream serialIn(serialPort); // 시리얼 포트를 읽기 모드로 열기
    if (!serialIn) {
        std::cerr << "Failed to open the serial port." << std::endl;
        return 1;
    }

    serialIn.

    while (true) {
        std::string data;
        std::string line;

        std::cout << "Enter data to send to Arduino: ";
        std::cin >> data;

        serialOut << data << std::endl; // 데이터를 시리얼 포트로 전송

        char receivedChar;
        std::string buffer = ""; // 데이터를 저장할 문자열 버퍼

        serialIn.get(receivedChar); // 시리얼 포트에서 문자 읽기

        if (receivedChar == '@') {
            buffer += receivedChar; // 데이터를 버퍼에 추가
            std::cout << buffer<< std::endl;
        } else if (receivedChar == '#'){
            // '#' 문자를 수신하면 버퍼에 저장된 데이터 출력
            std::cout << "Received Data: " << buffer << std::endl;
            buffer = ""; // 버퍼 비우기
        }

    }



    while (true) {

    }


    return 0;
}*/