$(document).ready(function(){
	refresh(function(){
		setInterval(setTime, 1000);
		refreshAlarm();
	});
	$('#fileSelectModal').on('show.bs.modal', function (e) {
		var targetInput = $(e.relatedTarget).data('inputtarget');
		$("#selectFileList").empty();
		$.ajax({
			url: "files.json",
		}).done(function(data){
				jQuery("<a/>",{class:"list-group-item list-group-item-action",href:"#",text:"Random"}).click(function(evt){
					$("#"+targetInput).val("");
					$('#fileSelectModal').modal('hide');
				}).appendTo("#selectFileList");
			for(i in data){
				jQuery("<a/>",{class:"list-group-item list-group-item-action",href:"#",text:data[i]}).data("file",data[i]).click(function(evt){
					$("#"+targetInput).val($(evt.target).data('file'));
					$('#fileSelectModal').modal('hide');
				}).appendTo("#selectFileList");
			}
		}).fail(function() {
			alert("error");
		});
	});
});
function refreshOnce(){
	refresh(function(){
		refreshAlarm();
	});
}
var defaultvalues = {
		"en":0,
		"h":"08:00",
		"w":[6,0],
		"a":{"src":"sd","href":""}
	};
var dayweek= ["Sunday","Monday","Tuesday","Wednesday","Thursday","Friday","Saturday"];
function pad(val) {
  var valString = val + "";
  if (valString.length < 2) {
    return "0" + valString;
  } else {
    return valString;
  }
}
function refreshAlarm(){
	$.ajax({
		url: "alarm.json",
	}).done(function(data){
		$("#mainalarms").empty();
		for(i in data){
			addTmpl(data[i]);
		}
	}).fail(function() {
		alert("error");
	});
}
function setTime() {
	var seconds = $("#hour").data("sec");
	var minutes = $("#hour").data("min");
	var hours = $("#hour").data("hour");
	var weekday = $("#hour").data("day");
	++seconds;
	if(seconds == 60){++minutes; seconds = 0;}
	if(minutes == 60){++hours; minutes = 0;}
	if(hours == 24){++weekday; hours = 0;}
	if(weekday == 7){weekday=0;}
	$("#hour").data("hour",hours).data("min",minutes).data("sec",seconds).data("day",weekday);
	var hourText = pad($("#hour").data("hour"))+":"+pad($("#hour").data("min"))+":"+pad($("#hour").data("sec"));
	$("#hour").text(hourText);
	$("#weekday").text(dayweek[$("#hour").data("day")]);
}
function trueFalseHtml(bool) {
	if(bool){
		return "<span class=\"badge badge-success\">TRUE</span>";
	} else {
		return "<span class=\"badge badge-danger\">FALSE</span>";
	}
}
function refresh(cb){
	$.ajax({
		url: "status.json",
	}).done(function(data){
		$("#hour").data("hour",data.hour).data("min",data.min).data("sec",data.sec).data("day",data.day);
		$("#weekday").text(dayweek[$("#hour").data("day")]);
		$("#alarmstate").html(trueFalseHtml(data.alarmstate));
		$("#ssid").text(data.ssid);
		$("#sdcardpresence").html(trueFalseHtml(data.sdcardpresence));
		$("#sdloaded").html(trueFalseHtml(data.sdloaded));
		cb();
	}).fail(function() {
		alert("error");
	});
}
function addTmpl(values){
	var id = $(".alarmcol").length;
	values["id"] = id;
	var alarmdom = $.parseHTML(tmpl("alarmtmpl",values));
	$(alarmdom).find(".removeBtn").click(function(evt){
		$(evt.target).parents(".alarmcol").remove();
	});
	if(values.en == 1){
		$(alarmdom).find("#active-"+id).prop('checked', true);
		$(alarmdom).find(".card").addClass("border-success");
	}
	$(alarmdom).find("#active-"+id).change(function(evt){
		if($(evt.target).prop("checked") == true){
			$(evt.target).parents(".card").addClass("border-success");
		} else {
			$(evt.target).parents(".card").removeClass("border-success");
		}
	});
	for(wd in values.w){
		$(alarmdom).find("#day-"+values.w[wd]+"-"+id).prop('checked', true);
	}
	function switchAlarmType(dom,target){
		$(dom).find(".typeselection").children().hide();
		$(dom).find("."+target).show();
	}
	$(alarmdom).find("input[data-target='"+values.a.src+"']").prop('checked', true);
	switchAlarmType(alarmdom,$(alarmdom).find(".alarmtype").data("src"));
	$(alarmdom).find("input[name='typeselect-"+id+"']").change(function(evt){
		switchAlarmType(alarmdom,$(evt.target).data("target"));
	});
	$(alarmdom).appendTo("#mainalarms");
}
function addAlarm(){
	addTmpl(defaultvalues);
}
function validate(){
	$("#saveBtn").addClass("disabled");
	$("#saveBtn").text("Saving...");
	var output = [];
	$(".alarmcol").each(function(index,elem){
		var thisalarm = {};
		var id = $(elem).data("id");
		thisalarm.en = $(elem).find("#active-"+id).prop('checked') ? 1 : 0;
		thisalarm.h = $(elem).find("#hour-"+id).val();
		thisalarm.w = [];
		for(var wd = 0; wd < 7;wd++){
			if($(elem).find("#day-"+wd+"-"+id).prop('checked')){
				thisalarm.w.push(wd);
			}
		}
		if($("#webradio-"+id).prop('checked')){
			thisalarm.a = {"src":"url","href":$("#stream-url-"+id).val()}
		} else if($("#sdfile-"+id).prop('checked')){
			thisalarm.a = {"src":"sd","href":$("#file-"+id).val()}
		}
		output.push(thisalarm);
	});
	$.ajax({
		url: "alarm.json",
		method: "POST",
		data: JSON.stringify(output)
	}).done(function(data){
		$("#saveBtn").removeClass("disabled");
		$("#saveBtn").text("Save");
	}).fail(function() {
		alert("error");
	});
}