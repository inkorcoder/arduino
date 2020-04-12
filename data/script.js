// var HOST = "http://192.168.0.160/";
var HOST = "/";

function z(v){
	return v < 10 ? "0"+v : v;
}

function fireOnChange(element){
	var e = null;
	if (document.createEventObject) {
		e = document.createEventObject();
		element.fireEvent('oninput', e);
	} else {
		var event = document.createEvent('Event');
		event.initEvent('input', true, true);
		element.dispatchEvent(event);
	}
}

window.waitForFinalEvent = (function () {
	var timers = {};
	return function (callback, ms, uniqueId) {
		if (!uniqueId) {
			uniqueId = "Don't call this twice without a uniqueId";
		}
		if (timers[uniqueId]) {
			clearTimeout(timers[uniqueId]);
		}
		timers[uniqueId] = setTimeout(callback, ms);
	};
})();

// console.log()
$('.anchor').click(function(e){
	$('.anchor').not(this).removeClass('active');
	this.toggleClass('active');
});

var options = "";
for (var i = -30; i < 30; i++){
	options += "<option value=\""+i+"\" "+(i === 0 ? 'selected' : '')+">"+i+"</option>";
}
$('#hours, #minutes, #seconds').html(options);

options = "";
for (var i = 0; i < 10; i++){
	options += "<option value=\""+i+"\" "+(i === 0 ? 'selected' : '')+">"+i+"</option>";
}
$('#selectedGroup, #selectedSymbol').html(options);

$('#hours, #minutes, #seconds').on('change', function(){
	console.log(this.value);
});

$('.input-range').each(function(i, range){
	$('input', range).on('input', function(){
		$('small', range.parentNode.parentNode).html(this.value);
	});
});

$('[data-activate-by]').each(function(i, el){
	$(el.getAttribute('data-activate-by')).on('change', function(){
		$(el).removeClass('hidden');
	});
	$(el.getAttribute('data-deactivate-by')).on('change', function(){
		$(el).addClass('hidden');
	});
});


for (var i = 0; i < 5; i++){
	var symbols = "";
	var s = ":.";
	var count = i == 2 ? 1 : 9;
	for (var j = count; j >= 0; j--){
		symbols += "<input type=\"radio\" name=\"use-symbol\" id=\"group"+(i)+"_symb"+(j)+"\"><label for=\"group"+(i)+"_symb"+(j)+"\">"+(i == 2 ? s[j] : j)+"</label>";
	}
	$('#symbol'+(i+1)+'Group').html(symbols);
}


$('.tabs li').click(function(e){
	var i = $(this).index();
	$('li', this.parentNode).removeClass('active');
	$('.bg .fader').removeClass('active');
	$('.tab').removeClass('active');
	$(this).addClass('active');
	// $('[name="theme-color"]')[0].setAttribute("content", this.getAttribute('data-color'));
	$('.bg .f'+(i+1)).addClass('active');
	$('.tab.t'+(i+1)).addClass('active');
});

$('#nightMode input').click(function(e){
	console.log('setting night mode to '+e.target.value);
	$.ajax({url: HOST+'night-mode?enabled='+e.target.value}).done(function(res){
		console.log(res);
	});
});

$('#displayMode input').click(function(e){
	console.log('setting dosplay mode to '+e.target.value);
	$.ajax({url: HOST+'display-mode?mode='+e.target.value}).done(function(res){
		console.log(res);
	});
});

var globalColor = [0,0,0];
// $('.main')[0].setAttribute('style', "color: rgb("+globalColor.join(',')+")");

$('#red, #green, #blue').each(function(index, input){
	input.value = globalColor[index];
	input.addEventListener('input', function(e){
		waitForFinalEvent(function(){
			// console.log(e.target.value);
			globalColor[index] = e.target.value;
			$('.main')[0].setAttribute('style', "color: rgb("+globalColor.join(',')+")");
			$.ajax({url: HOST+'global-color?r='+globalColor[0]+'&g='+globalColor[1]+'&b='+globalColor[2]}).done(function(res){

			});
		}, 300, '');
	});
});


function renderDateTime(){
	var now = new Date();
	$('#h').html(z(now.getHours()));
	$('#m').html(z(now.getMinutes()));
	$('#s').html(z(now.getSeconds()));
	$('.date').html(z(now.getUTCDate())+"."+z(now.getMonth()+1)+"."+now.getUTCFullYear());
}
renderDateTime();
setInterval(function(){
	renderDateTime();
}, 1000);


var now = new Date();
$.ajax({url:
	HOST+'synchronize-time?Y='+now.getFullYear()+'&M='+(now.getMonth()+1)+'&D='+now.getUTCDate()+'&h='+now.getHours()+'&m='+now.getMinutes()+'&s='+now.getSeconds()
}).done(function(res){});

$.ajax({url: HOST+'get-settings'}).done(function(res){
	var settings = JSON.parse(res.replace(/\'/gim, "\""));
	globalColor[0] = settings.color.r;
	globalColor[1] = settings.color.g;
	globalColor[2] = settings.color.b;
	$('#red')[0].value = settings.color.r;
	$('#green')[0].value = settings.color.g;
	$('#blue')[0].value = settings.color.b;
	fireOnChange($('#red')[0]);
	fireOnChange($('#green')[0]);
	fireOnChange($('#blue')[0]);
	$('.main')[0].setAttribute('style', "color: rgb("+globalColor.join(',')+")");
	$('input[name="night-mode"], input[name="display-mode"]').each((function(index, input){
		input.removeAttribute("checked");
	}));
	$('input[name="night-mode"][value="'+settings.nightMode+'"]')[0].setAttribute('checked', "true");
	$('input[name="display-mode"][value="'+settings.displayMode+'"]')[0].setAttribute('checked', "true");
	console.log(settings);
});


function getCredentials(){
	return {
		ssid: $('#ssid').val().trim(),
		password: $('#password').val().trim()
	};
}

$("#connect").click(function(){
	var data = getCredentials();
	$.ajax({url: HOST+'set_credentials?ssid='+data.ssid+'&password='+data.password}).done(function(res){
		console.log(res);
		alert("Clock connected to your network.\nYou can connect to your network and go to configure clock.\nIP: http://192.168.0.160");
	});
});
$("#resetConnection").click(function(){
	$.ajax({url: HOST+'reset_connection'}).done(function(res){
		console.log(res);
		alert("Clock is disconnected from your network.\nFind \"Clock-Hotspot\" in your wifi networks to connect and reconfigure.\n IP: http://192.168.4.1");
	});
});