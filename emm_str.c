union _key{
    uint8_t _bkey[16];
    uint32_t _ukey[4];
};

uint8_t emmbuffer[64] ={
0x12, 0x4E, 0x82, 0x70, 0x3B, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x70, 0x42, 0x64, 0x10,
0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC,
0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC,
0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC
};

uint8_t syskey[16];
uint8_t omac[16];
uint8_t enckey[16];
uint8_t key20[16];
uint8_t key21[16];
uint8_t datum[2];




//uint8_t client_id[4];

uint8_t client_id[4];


uint8_t dec2bcd_r(uint16_t dec)
{
    return (dec) ? ((dec2bcd_r( dec / 10 ) << 4) + (dec % 10)) : 0;
}

uint16_t _emmdate(int inday,int inmon, int inyear){


    union _CNX_DATE _conaxdate;

    _conaxdate._bit._tenyear = (inyear - 1990) / 10;
    _conaxdate._bit._years = (inyear - 1990) % 10;
    _conaxdate._bit._day = inday;
    _conaxdate._bit._month = inmon;

    return _conaxdate._outdate;
}


uint16_t _conax_datum(uint8_t debug){
    time_t rawtime; // = time(NULL);
    struct tm *ltm;// = localtime(&rawtime);

    rawtime = time(NULL);
    ltm = localtime(&rawtime);

    union _CNX_DATE _conaxdate;
    if(debug == 1){
        printf("Local-time: %02d.%02d.%04d %02d:%02d:%02d ", ltm->tm_mday, ltm->tm_mon +1 , ltm->tm_year + 1900, ltm->tm_hour, ltm->tm_min, ltm->tm_sec);
        ltm->tm_isdst ? printf("DST\n\n") : printf("\n\n");
    }
    _conaxdate._bit._tenyear = ((ltm->tm_year + 1900) - 1990) / 10;
    _conaxdate._bit._years = ((ltm->tm_year + 1900) - 1990) % 10;
    _conaxdate._bit._day = ltm->tm_mday;
    _conaxdate._bit._month = ltm->tm_mon +1;

    return _conaxdate._outdate;
}

uint16_t _conax_time(void){
    time_t rawtime; // = time(NULL);
    struct tm *ltm;// = localtime(&rawtime);

    union _zeit{
        uint16_t _16;
        uint8_t _8[2];
    };

    union _zeit _xx;
    rawtime = time(NULL);
    ltm = localtime(&rawtime);
    _xx._8[1] = dec2bcd_r(ltm->tm_min);
    _xx._8[0] = dec2bcd_r(ltm->tm_sec);
    return _xx._16;
}


void read_syskey_sql(uint8_t debug,const char *database,const char *user,const char *pass,const char *dbname){
    MYSQL *con = mysql_init(NULL);
    if (con == NULL){
        fprintf(stderr, "mysql_init() failed\n");
        exit(1);
    }

    if (mysql_real_connect(con, database, user, pass, dbname, 0, NULL, 0) == NULL){
        finish_with_error(con);
    }

    if (mysql_query(con, "SELECT systemkey FROM emmg_systemkey where id=1")){
        finish_with_error(con);
    }

    MYSQL_RES *result = mysql_store_result(con);

    if (result == NULL){
        finish_with_error(con);
    }

    int num_fields = mysql_num_fields(result);

    MYSQL_ROW row;
    row = mysql_fetch_row(result);

    if(debug ==1){
        printWithTime("");printf("Systemkey : ");
    }

    for(int idx = 0;idx < 16;idx++){
        sscanf((const char*)row[0]+idx*2,"%02hhx",&syskey[idx]);
        if(debug ==1){
            printf("%02X ",syskey[idx]);
        }
    }
    if(debug ==1){
        printf("\n");
    }
    mysql_free_result(result);
    mysql_close(con);
}


void read_client_sql(uint8_t debug,const char *database,const char *user,const char *pass,const char *dbname){
    MYSQL *con = mysql_init(NULL);
    if (con == NULL){
        fprintf(stderr, "mysql_init() failed\n");
        exit(1);
    }

    if (mysql_real_connect(con, database, user, pass, dbname, 0, NULL, 0) == NULL){
        finish_with_error(con);
    }

    if (mysql_query(con, "SELECT client_id FROM emmg_systemkey where id=1")){
        finish_with_error(con);
    }

    MYSQL_RES *result = mysql_store_result(con);

    if (result == NULL){
        finish_with_error(con);
    }

    int num_fields = mysql_num_fields(result);

    MYSQL_ROW row;
    row = mysql_fetch_row(result);

    if(debug ==1){
        printWithTime("");printf("Client_ID : ");
    }

    for(int idx = 0;idx < 4;idx++){
        sscanf((const char*)row[0]+idx*2,"%02hhx",&client_id[idx]);
        if(debug ==1){
            printf("%02X ",client_id[idx]);
        }
    }
    if(debug ==1){
        printf("\n");
    }
    mysql_free_result(result);
    mysql_close(con);
}








void read_keys_sql(uint8_t debug,const char *database,const char *user,const char *pass,const char *dbname){
    MYSQL *con = mysql_init(NULL);
    MYSQL_RES *result;
    MYSQL_ROW row;

    if (con == NULL){
        fprintf(stderr, "mysql_init() failed\n");
        exit(1);
    }

    if (mysql_real_connect(con, database, user, pass, dbname, 0, NULL, 0) == NULL){
        finish_with_error(con);
    }

    if (mysql_query(con, "SELECT ecmkey FROM ecmg_keys where id=20")){
        finish_with_error(con);
    }

     result = mysql_store_result(con);

    if (result == NULL){
        finish_with_error(con);
    }

    row = mysql_fetch_row(result);

    if(debug ==1){
        //printWithTime("");printf("Key 20    : ");
		printWithTime("");printf("ECM-Key 20: ");

    }

    for(int idx = 0;idx < 16;idx++){
        sscanf((const char*)row[0]+idx*2,"%02hhx",&key20[idx]);
        if(debug ==1){
            printf("%02X ",key20[idx]);
        }
    }
    if(debug ==1){
        printf("\n");
    }
    
    if (mysql_query(con, "SELECT ecmkey FROM ecmg_keys where id=21")){
        finish_with_error(con);
    }
    result = mysql_store_result(con);
    if (result == NULL){
        finish_with_error(con);
    }
    row = mysql_fetch_row(result);
    if(debug ==1){
        //printWithTime("");printf("Key 21    : ");
		printWithTime("");printf("ECM-Key 20: ");
    }

    for(int idx = 0;idx < 16;idx++){
        sscanf((const char*)row[0]+idx*2,"%02hhx",&key21[idx]);
        if(debug ==1){
            printf("%02X ",key21[idx]);
        }
    }
    if(debug ==1){
        printf("\n");
    }

    mysql_free_result(result);
    mysql_close(con);
}


uint8_t generate_emm(char *in,uint8_t *out,uint8_t debug){

    uint32_t ppua,ppsa;//,temp[2][2];
    int day,mon,year,day2,mon2,year2;
    uint32_t acc;
    char name[20] ={0};
    uint32_t sid;
    uint8_t pp[4],idx;
    uint16_t time = _conax_time();

    union _emm{
        uint8_t _in[0x30];
        uint32_t _xt[6][2];
    } emm;


    memcpy(&out[0],&emmbuffer[0],64);
        //#if ENABLE_UNIQUE
        if(in[0] == 0x55){
			
            sscanf(in,"U:%d S:%d-%d-%d E:%d-%d-%d A:%08x N:%s SA:%d ID:%04x",&ppua,&year,&mon,&day,&year2,&mon2,&day2,&acc,&name[0],&ppsa,&sid);
            if(debug == 1){
				printWithTime("");printf("++++++++++++++++++++++++++\n");
				printWithTime("");printf("+      UNIQUE EMM        +\n");
				printWithTime("");printf("++++++++++++++++++++++++++\n");
                printWithTime("");printf("Serial-No      : %d\n",ppua);
				printWithTime("");printf("PPUA           : %08X\n",ppua);
                printWithTime("");printf("Start-date     : %02d.%02d.%d\n",day,mon,year);
                printWithTime("");printf("End-date       : %02d.%02d.%d\n",day2,mon2,year2);
                printWithTime("");printf("Access Criteria: %08X\n",acc);
                printWithTime("");printf("PPSA Update    : %08X\n",ppsa);
                printWithTime("");printf("SERVICE-ID     : %04X\n",sid);
            }

            /* FILL EMM BUFFER */
            idx = 16;
            out[idx] = time >> 8 & 0xff;idx++;
            out[idx] = time & 0xff;idx++;
            out[idx] = 0xA0;idx++;
            out[idx] = 0x00;idx++;
            out[8] = out[idx] = ppua >> 24 & 0xff;idx++;
            out[9] = out[idx] = ppua >> 16 & 0xff;idx++;
            out[10] = out[idx] = ppua >> 8 & 0xff;idx++;
            out[11] = out[idx] =  ppua & 0xff;idx++;
            out[idx] = 0xA0;idx++;
            out[idx] = 0x03;idx++;
            out[idx] = _emmdate(day,mon,year) >> 8 & 0xff;idx++;
            out[idx] = _emmdate(day,mon,year) & 0xff;idx++;
            out[idx] = _emmdate(day2,mon2,year2) >> 8 & 0xff;idx++;
            out[idx] = _emmdate(day2,mon2,year2) & 0xff;idx++;
            out[idx] = 0xA0;idx++;
            out[idx] = 0x04;idx++;
            out[idx] = acc >> 24 & 0xff;idx++;
            out[idx] = acc >> 16 & 0xff;idx++;
            out[idx] = acc >> 8 & 0xff;idx++;
            out[idx] = acc & 0xff;idx++;
            if(strlen(name)>=1){
                    if(debug == 1){
                        printWithTime("");printf("Name           : %s\n",name);
                    }
                    out[idx] = 0xA0;idx++;
                    out[idx] = 0x10;idx++;
                    memset(&out[idx],0x20,0x0f);
                    memcpy(&out[idx],&name[0],strlen(name));
                    idx += 15;
            }
            out[idx] = 0xA0;idx++;
            out[idx] = 0x02;idx++;
            out[idx] = ppsa >> 24 & 0xff;idx++;
            out[idx] = ppsa >> 16 & 0xff;idx++;
            out[idx] = ppsa >> 8 & 0xff;idx++;
            out[idx] = ppsa & 0xff;idx++;
            out[idx] = 0xA0;idx++;
            out[idx] = 0x01;idx++;
            out[idx] = sid >> 8 & 0xff;idx++;
            out[idx] = sid & 0xff;idx++;

            memset(&name,0,20);

            pp[0] = ppua >> 24 & 255;
            pp[1] = ppua >> 16 & 255;
            pp[2] = ppua >> 8 & 255;
            pp[3] = ppua & 255;

            AES_CMAC(syskey,pp,0x4,enckey);
            if(debug == 1){
				printWithTime("");printf("Client ID      : ");
                for(int i=0;i<4;i++){
                    printf("%02x ",client_id[i]);
                }
                printf("\n");
                printWithTime("");printf("ENC-KEY        : ");
                for(int i=0;i<16;i++){
                    printf("%02x ",enckey[i]);
                }
                printf("\n");
            }
			
         }
		 //#endif
        //#if ENABLE_SHARED
        if(in[0] == 0x47){
             
            sscanf(in,"G:%d",&ppsa);
			 
            if(debug ==1){
                printWithTime("");printf("++++++++++++++++++++++++++\n");
				printWithTime("");printf("+      SHARED EMM        +\n");
				printWithTime("");printf("++++++++++++++++++++++++++\n");
                printWithTime("");printf("Shared Address : %d \n" ,ppsa,ppsa);
				printWithTime("");printf("PPSA           : %08X\n",ppsa,ppsa);
            }
            idx = 16;
            out[idx] = time >> 8 & 0xff;idx++;
            out[idx] = time & 0xff;idx++;
            out[idx] = 0xA0;idx++;
            out[idx] = 0x02;idx++;
            out[8] = out[idx] = ppsa >> 24 & 0xff;idx++;
            out[9] = out[idx] = ppsa >> 16 & 0xff;idx++;
            out[10] = out[idx] = ppsa >> 8 & 0xff;idx++;
            out[11] = out[idx] =  ppsa & 0xff;idx++;
            out[idx] = 0xA0;idx++;
            out[idx] = 0x20;idx++;
            memcpy(&out[idx],key20,16);
            idx += 16;
            out[idx] = 0xA0;idx++;
            out[idx] = 0x21;idx++;
            memcpy(&out[idx],key21,16);

            pp[0] = ppsa >> 24 & 255;
            pp[1] = ppsa >> 16 & 255;
            pp[2] = ppsa >> 8 & 255;
            pp[3] = ppsa & 255;

            AES_CMAC(syskey,pp,0x4,enckey);

            if(debug ==1){
				printWithTime("");printf("Client ID      : ");
                for(int i=0;i<4;i++){
                    printf("%02x ",client_id[i]);
                }
                printf("\n");
                printWithTime("");printf("ENC-KEY        : ");
                for(int i=0;i<16;i++){
                    printf("%02x ",enckey[i]);
                }
                printf("\n");
            }
			
        }
		//#endif
           //calculate key AES CMAC
            memcpy(&emm._in[0],&out[16],0x30);

            AES_CMAC(enckey,emm._in,0x30,omac);
            if(debug ==1){
                printWithTime("");printf("OMAC           : ");
                for(int i=0;i<16;i++){
                    printf("%02x ",omac[i]);
                    //out[64+i] = omac[i];
                }
                printf("\n");
            }

            for(int i=0;i<16;i++){
                out[64+i] = omac[i];
            }



            aes_cbc(enckey,emm._in,0x30);
            memcpy(&out[16],&emm._in[0],0x30);
            if(debug == 1){
				printWithTime("");printf("EMM to SEND    : ");

				
				for(int i = 0;i < 64;i++){
					if (i % 16 == 0)
			{
             printf("\n");
             printWithTime("               : ");
            }
					printf("%02x ",out[i]);}
				
				printf("\n");

		
            }
    return 64;
}

