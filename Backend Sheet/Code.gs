var sheet_id = "1aHFPdMdhAe6RIfowm4y4HkUbjac_GVULG4P3ONsJ5so";

var sheet_name = "nodeA";
function doGet(e){
var ss = SpreadsheetApp.openById(sheet_id);
var sheet = ss.getSheetByName(sheet_name);
// var Day_time= e.parameter.today;
var Day_time = new Date().toLocaleString();
var SF = Number(e.parameter.SF);
var txP= Number(e.parameter.txP);
var tolRxNum = Number(e.parameter.tolRxNum);
var avgRxSize = Number(e.parameter.avgRxSize);
var avgRssi = Number(e.parameter.avgRssi);
var maxRssi = Number(e.parameter.maxRssi);
var minRssi = Number(e.parameter.minRssi);
var avgtoa = Number(e.parameter.avgtoa);
var minton = Number(e.parameter.minton);
var maxton = Number(e.parameter.maxton);
var nodeId = Number(e.parameter.nodeId);
var rxNodeId = Number(e.parameter.rxNodeId);
var avgSNR = Number(e.parameter.avgSNR);
var avgFrequencyError = Number(e.parameter.avgFrequencyError);

// sheet.appendRow([prop,val,date]);
sheet.appendRow([Day_time,SF,txP,tolRxNum,avgRssi,maxRssi,minRssi,avgtoa,minton,maxton,avgSNR,avgFrequencyError,avgRxSize,nodeId,rxNodeId]);


return ContentService.createTextOutput('success');

}