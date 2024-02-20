var socket = io.connect('http://localhost:5000');
        // listen for mqtt_message events
        // when a new message is received, log data to the page

$(document).ready(function() {
    var socket = io.connect(); 
    $('input[type="checkbox"]').change(function() {
        var value = $(this).is(":checked") ? 1 : 0;
        socket.emit('checkbox value', value);
        console.log("gia tri: ",value);
    });
});

$(document).ready(function() {
    $('input[type="date"]').change(function() {
      var selectedDate = $(this).val();
      console.log("Selected date: " + selectedDate);
      socket.emit('date selected', String(selectedDate));
    });
  });


function UpdateRowTable(index,object){
    document.getElementById('time'+index.toString()).innerHTML = object.time.toString();
    document.getElementById('date'+index.toString()).innerHTML = object.date.toString();
    if(object.name.toString() != undefined)
        document.getElementById('name'+index.toString()).innerHTML = object.name.toString();
    else document.getElementById('name'+index.toString()).innerHTML = "---";
};      

socket.on('update_on_visited', function(data) {
    console.log("chuoi JSON nhan duoc: ",data)
    myarray = JSON.parse(data);
    $('#table tbody').empty();
    $.each(myarray, function(index, row) {
      $('#table tbody').append('<tr><td>' + row.time + '</td><td>' + row.date + '</td><td>' + row.name + '</td></tr>');
    });
  });

socket.on('update_image', function(data) {
    $('#myimage').attr('src', "{{url_for('static', filename='temp_image.jpg')}}");
});

socket.on('update_on_click',function(data){
    console.log("chuoi JSON nhan duoc: ",data)
    myarray = JSON.parse(data);
    var index = 0;
    myarray.forEach(function(item){
        UpdateRowTable(index,item);
        index++;
    })
})

socket.on('updatethongbao',function(data){
    document.getElementById("lcdhere").innerHTML = data;
})

socket.emit('connect_homepage','connected')

var button = document.getElementById("ChangePwdBtn");
  button.addEventListener("click", function() {
    console.log("nhan doi mk ");
    socket.emit('let_change_Pwd','changePwd');
    button.disabled = true; // Ngăn người dùng nhấn nút
    // Thực hiện các thao tác cần thiết
    setTimeout(function() {
      button.disabled = false; // Cho phép người dùng nhấn lại nút
    }, 200); // Thời gian giữ nguyên trạng thái của nút sau 200ms giây
  });


