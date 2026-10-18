/* 20250414 Ver.310 - 20260921 Ver.335 */
/* js.js */

/* (1)定期的に関数を実行 */
/* Server(cpp) で生成した時刻を id = curtime により共有して Client(js) で取得して表示する */
 var getCurTime = function () {
    var xhr = new XMLHttpRequest();
    xhr.onreadystatechange = function() {
      if (this.readyState == 4 && this.status == 200) {
        document.getElementById("curtime").innerHTML = this.responseText;
      }
    };
    xhr.open("GET", "/curtime", true);
    xhr.send(null);
    /* モード切換ボタンの有効・無効の切換を追加した */
    control_bt_mode_status();
  }
  /* 500ms 周期で実行する */
  setInterval(getCurTime, 500);
  
  /* Server(cpp) の情報を id = msg_information により共有して Client(js) で取得して表示する */
  var getMsg_information = function () {
    var xhr = new XMLHttpRequest();
    xhr.onreadystatechange = function() {
      if (this.readyState == 4 && this.status == 200) {
        document.getElementById("msg_information").innerHTML = this.responseText;
      }
    };
    xhr.open("GET", "/msg_information", true);
    xhr.send(null);
  }
  /* 200ms 周期で実行 */
  setInterval(getMsg_information, 200);
  
  /* (2)「リモコン送信ボタン」群 */
  /* TV */
  /*「電源」ボタンを押したときの動作を記述*/ 
  function bt_1Down(element){
    if (document.getElementById('bt_c1').checked == false){
      var xhr = new XMLHttpRequest();
      xhr.open("GET", "/bt_1_on", true);
      xhr.send(null); }
    }
  function bt_1Up(element){
    if (document.getElementById('bt_c1').checked == false){
      var xhr = new XMLHttpRequest();
      xhr.open("GET", "/bt_1_off", true);
      xhr.send(null); }
    }
 
  /*「入力」ボタンを押したときの動作を記述*/
  function bt_2Down(element){
    if (document.getElementById('bt_c1').checked == false){
      var xhr = new XMLHttpRequest();
      xhr.open("GET", "/bt_2_on", true);
      xhr.send(null); }
    }
  function bt_2Up(element){
    if (document.getElementById('bt_c1').checked == false){
      var xhr = new XMLHttpRequest();
      xhr.open("GET", "/bt_2_off", true);
      xhr.send(null); }
    }

  /*「消音」ボタンを押したときの動作を記述*/
  function bt_3Down(element){
    if (document.getElementById('bt_c1').checked == false){
      var xhr = new XMLHttpRequest();
      xhr.open("GET", "/bt_3_on", true);
      xhr.send(null); }
    }
  function bt_3Up(element){
    if (document.getElementById('bt_c1').checked == false){
      var xhr = new XMLHttpRequest();
      xhr.open("GET", "/bt_3_off", true);
      xhr.send(null); }
    }

  /*「予備_bt_4」ボタンを押したときの動作を記述*/
  function bt_4Down(element){
    if (document.getElementById('bt_c1').checked == false){
      var xhr = new XMLHttpRequest();
      xhr.open("GET", "/bt_4_on", true);
      xhr.send(null); }
    }
  function bt_4Up(element){
    if (document.getElementById('bt_c1').checked == false){
      var xhr = new XMLHttpRequest();
      xhr.open("GET", "/bt_4_off", true);
      xhr.send(null); }
    }

  /*「CH+」ボタンを押したときの動作を記述*/
  function bt_5Down(element){
    if (document.getElementById('bt_c1').checked == false){
      var xhr = new XMLHttpRequest();
      xhr.open("GET", "/bt_5_on", true);
      xhr.send(null); }
    }
  function bt_5Up(element){
    if (document.getElementById('bt_c1').checked == false){
      var xhr = new XMLHttpRequest();
      xhr.open("GET", "/bt_5_off", true);
      xhr.send(null); }
    }

  /*「地上」ボタンを押したときの動作を記述*/
  function bt_6Down(element){
    if (document.getElementById('bt_c1').checked == false){
      var xhr = new XMLHttpRequest();
      xhr.open("GET", "/bt_6_on", true);
      xhr.send(null); }
    }
  function bt_6Up(element){
    if (document.getElementById('bt_c1').checked == false){
      var xhr = new XMLHttpRequest();
      xhr.open("GET", "/bt_6_off", true);
      xhr.send(null); }
    }

  /*「Vol+」ボタンを押したときの動作を記述*/
  function bt_7Down(element){
    if (document.getElementById('bt_c1').checked == false){
      var xhr = new XMLHttpRequest();
      xhr.open("GET", "/bt_7_on", true);
      xhr.send(null); }
    }
  function bt_7Up(element){
    if (document.getElementById('bt_c1').checked == false){
      var xhr = new XMLHttpRequest();
      xhr.open("GET", "/bt_7_off", true);
      xhr.send(null); }
    }

  /*「予備_bt_8」ボタンを押したときの動作を記述*/
  function bt_8Down(element){
    if (document.getElementById('bt_c1').checked == false){
      var xhr = new XMLHttpRequest();
      xhr.open("GET", "/bt_8_on", true);
      xhr.send(null); }
    }
  function bt_8Up(element){
    if (document.getElementById('bt_c1').checked == false){
      var xhr = new XMLHttpRequest();
      xhr.open("GET", "/bt_8_off", true);
      xhr.send(null); }
    }

  /*「CHｰ」ボタンを押したときの動作を記述*/
  function bt_9Down(element){
    if (document.getElementById('bt_c1').checked == false){
      var xhr = new XMLHttpRequest();
      xhr.open("GET", "/bt_9_on", true);
      xhr.send(null); }
    }
  function bt_9Up(element){
    if (document.getElementById('bt_c1').checked == false){
      var xhr = new XMLHttpRequest();
      xhr.open("GET", "/bt_9_off", true);
      xhr.send(null); }
    }

  /*「BS」ボタンを押したときの動作を記述*/
  function bt_10Down(element){
    if (document.getElementById('bt_c1').checked == false){
      var xhr = new XMLHttpRequest();
      xhr.open("GET", "/bt_10_on", true);
      xhr.send(null); }
    }
  function bt_10Up(element){
    if (document.getElementById('bt_c1').checked == false){
      var xhr = new XMLHttpRequest();
      xhr.open("GET", "/bt_10_off", true);
      xhr.send(null); }
    }

  /*「Vol-」ボタンを押したときの動作を記述*/
  function bt_11Down(element){
    if (document.getElementById('bt_c1').checked == false){
      var xhr = new XMLHttpRequest();
      xhr.open("GET", "/bt_11_on", true);
      xhr.send(null); }
    }
  function bt_11Up(element){
    if (document.getElementById('bt_c1').checked == false){
      var xhr = new XMLHttpRequest();
      xhr.open("GET", "/bt_11_off", true);
      xhr.send(null); }
    }

  /*「予備_bt_12」ボタンを押したときの動作を記述*/
  function bt_12Down(element){
    if (document.getElementById('bt_c1').checked == false){
      var xhr = new XMLHttpRequest();
      xhr.open("GET", "/bt_12_on", true);
      xhr.send(null); }
    }
  function bt_12Up(element){
    if (document.getElementById('bt_c1').checked == false){
      var xhr = new XMLHttpRequest();
      xhr.open("GET", "/bt_12_off", true);
      xhr.send(null); }
    }

  /* BOSE */
  /*「On」ボタン bt_13 を押したときの動作を記述*/
  function bt_13Down(element){
     /* bt_c1 「取込」ボタンが有効な時は bt_13 を無効にする */
    if (document.getElementById('bt_c1').checked == false){
      var xhr = new XMLHttpRequest();
      xhr.open("GET", "/bt_13_on", true);
      xhr.send(null); }
    }
  function bt_13Up(element){
    if (document.getElementById('bt_c1').checked == false){
      var xhr = new XMLHttpRequest();
      xhr.open("GET", "/bt_13_off", true);
      xhr.send(null); }
    }

  /*「Off」ボタンを押したときの動作を記述*/
  function bt_14Down(element){
    if (document.getElementById('bt_c1').checked == false){
      var xhr = new XMLHttpRequest();
      xhr.open("GET", "/bt_14_on", true);
      xhr.send(null); }
    }
  function bt_14Up(element){
    if (document.getElementById('bt_c1').checked == false){
      var xhr = new XMLHttpRequest();
      xhr.open("GET", "/bt_14_off", true);
      xhr.send(null); }
    }

  /*「Vol+」ボタンを押したときの動作を記述*/
  function bt_15Down(element){
    if (document.getElementById('bt_c1').checked == false){
      var xhr = new XMLHttpRequest();
      xhr.open("GET", "/bt_15_on", true);
      xhr.send(null); }
    }
  function bt_15Up(element){
    if (document.getElementById('bt_c1').checked == false){
      var xhr = new XMLHttpRequest();
      xhr.open("GET", "/bt_15_off", true);
      xhr.send(null); }
    }

  /*「Vol-」ボタンを押したときの動作を記述*/
  function bt_16Down(element){
    if (document.getElementById('bt_c1').checked == false){
      var xhr = new XMLHttpRequest();
      xhr.open("GET", "/bt_16_on", true);
      xhr.send(null); }
    }
  function bt_16Up(element){
    if (document.getElementById('bt_c1').checked == false){
      var xhr = new XMLHttpRequest();
      xhr.open("GET", "/bt_16_off", true);
      xhr.send(null); }
    }
 
  /* (3)モード切換ボタン */
  /* モード切換ボタンにより「取込」「登録」「終了」「予備」ボタンの有効/無効がトグルする */
  function bt_modeClick(){
    var xhr = new XMLHttpRequest();
    xhr.open("GET", "/mode", true);
    xhr.send(null); 
  }
  
  /* (4)制御ボタン群 */
  /*「取込」ボタンを押したときの動作を記述*/
  function bt_c1Click(){
    var xhr = new XMLHttpRequest();
    xhr.open("GET", "/capture", true);
    xhr.send(null);
    const b11 = document.getElementById('bt_c1');
    const b12 = document.getElementById('bt_c2');
    b11.checked = true; /* 「取込」ボタンONを保持する */
    b12.checked = false; /* 「登録」ボタンをOFFする */
    bt_active('bt_c1_label'); /* 「取込」ボタンをアクティブ色に変更 */
    bt_standby('bt_c2_label'); /* 「登録」ボタンをスタンバイ色に変更 */
    /* 16個の「リモコン送信ボタン」をすべて禁止する */
    for (let i = 1; i < 17; i++){ 
      buf_btc1 = "bt_" + i ;
      disabale_button(buf_btc1);
    }
  }
  
  /*「登録」ボタンを押したときの動作を記述*/
  function bt_c2Click(){
    var xhr = new XMLHttpRequest();
    xhr.open("GET", "/memory", true);
    xhr.send(null);
    const b11 = document.getElementById('bt_c1');
    const b12 = document.getElementById('bt_c2');
    b11.checked = false; /* 「取込」ボタンをOFFする */
    b12.checked = true; /* 「登録」ボタンONを保持する */
    bt_standby('bt_c1_label'); /* 「取込」ボタンをスタンバイ色に変更 */
    bt_active('bt_c2_label'); /* 「登録」ボタンをアクティブ色に変更 */
    enable_all_of_TX_keys(); /* 16個の「リモコン送信ボタン」をすべて許可する */
  }
  
  /*「終了」ボタンを押したときの動作を記述*/
  function bt_c3Clicked(element) {
    var xhr = new XMLHttpRequest();
    xhr.open("GET", "/finish", true);
    xhr.send(null);
    const b11 = document.getElementById('bt_c1');
    const b12 = document.getElementById('bt_c2');
    b11.checked = false; /* 「取込」ボタンをOFFする */
    b12.checked = false; /* 「登録」ボタンをOFFする */
    bt_standby('bt_c1_label'); /* 「取込」ボタンをスタンバイ色に変更 */
    bt_standby('bt_c2_label'); /* 「登録」ボタンをスタンバイ色に変更 */
    enable_all_of_TX_keys(); /* 16個の「リモコン送信ボタン」をすべて許可する */
  }
  
  /*「予備」ボタンを押したときの動作を記述*/
  function bt_c4Click(){
    var xhr = new XMLHttpRequest();
    xhr.open("GET", "/bt_c4", true);
    xhr.send(null);
    const b14 = document.getElementById('bt_c4');
    b14.checked = true; /* 「予備」ボタンONを保持する */
  }
  
  /* (5)button_id で指定したボタンを無効にする */
  async function disabale_button(button_id){
    try{
      document.getElementById(button_id).setAttribute("disabled", true);
    } catch (err){
      console.error(err);
    }
  }
  
  /* (6)button_id で指定したボタンを有効にする */
  async function enable_button(button_id){
    try{
      document.getElementById(button_id).removeAttribute("disabled");
    } catch (err){
      console.error(err);
    }
  }
  
  /* (7)リモコン送信ボタン群を有効にする */
  function enable_all_of_TX_keys(){
    /* 16個の「リモコン送信ボタン」bt_1 から bt_16 を有効にする */
    for (let i = 1; i < 17; i++){
      buf_btc2 = "bt_" + i ;
      enable_button(buf_btc2);
    }
  }
  
  /* (8)モード切換ボタンの動作を定義する */
  /** イベントリスナで モード切換ボタンの状態を検出して制御ボタンを有効にするか、無効にするか切換える */
  function control_bt_mode_status(){ 
    const ev_SRC = document.getElementById('bt_mode');
    ev_SRC.addEventListener('click',
      function(event){
        enable_all_of_TX_keys();
        const btSRC = document.getElementById('bt_mode');
        if ( btSRC.checked ){ /** bt_mode が true の場合制御ボタンを有効にして画面上で見えるようにする */
          enable_button("bt_c1");
          enable_button("bt_c2");
          enable_button("bt_c3");
          enable_button("bt_c4");
          bt_standby('bt_c1_label');
          bt_standby('bt_c2_label');
          bt_standby('bt_c3');
          bt_standby('bt_c4_label');
          document.getElementById("msg_information").innerHTML = "赤外線リモコン信号取込機能が有効になりました。 ";
        } else { /** bt_mode が false の場合制御ボタンを無効にして画面上で見えなくする */
          disabale_button("bt_c1");
          disabale_button("bt_c2");
          disabale_button("bt_c3");
          disabale_button("bt_c4");
          bt_hidden('bt_c1_label');
          bt_hidden('bt_c2_label');
          bt_hidden('bt_c3');
          bt_hidden('bt_c4_label');
          document.getElementById("msg_information").innerHTML = "赤外線リモコン信号取込機能を終了しました。\n通常の赤外線リモコン送信機として機能します。";
        }
      }
    );
  }
  
  /* (9)状態に応じて制御ボタンの色を変更する関数 */
  /* active state */
  function bt_active(button_label_id){
    const b11L = document.getElementById(button_label_id);
    b11L.style.color = 'white';
    b11L.style.background = 'tomato';
    b11L.style.borderColor = '#627295';
  }
  /* standby state */
  function bt_standby(button_label_id){
    const b11L = document.getElementById(button_label_id);
    b11L.style.color = 'black';
    b11L.style.background = 'rgb(183,222,232)';
    b11L.style.borderColor = '#627295';
  }
  /* hidden state */
  function bt_hidden(button_label_id){
    const b11L = document.getElementById(button_label_id);
    b11L.style.color = 'aliceblue';
    b11L.style.background = 'aliceblue';
    b11L.style.borderColor = 'aliceblue';
  }