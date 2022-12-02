$(function(){
  // Connect and Get all Characteristics
  // Check if the radio is available
  navigator.bluetooth.getAvailability().then(isAvailable => {
  if(isAvailable) {
    //$('#myModal').modal(options)
    $("#connectDialog").show();
  }
  else {
    console.log("Web BLE Not Available");
    $("#noWebBluetooth").show();
  }

  // Load the Generated Settings Table
  $("#settings").load("settings.html");

  $("#saveButton").on('click', saveToFlash);
  });
});

function parametersChanged()
{
  // Allow Save to Flash
  $("#saveButton").prop('disabled', false);
}

function showLoader()
{
  $("#connectDialog").hide();
  $("#loadingDialog").show();
}

function btConnectionStatus(note)
{
  $("#statusNote").text(note);
}

function hideLoader()
{
  $("#loading").hide();
}

function setValues()
{

}

function updateParameter(name, promise, value)
{
  if(!promise) {
    console.log("no worky");
    return;
  }
  console.log("Updating " + name +  " = " + value);

  //promise.writeValue();
  parametersChanged();
}

function saveToFlash()
{
  console.log("Saving to Flash");
  let encoder = new TextEncoder();
  commandCharacteristic.writeValue(encoder.encode("Flash"))
  .then(_ => {
    console.log("Wrote Flash to Command Characteristic");
  })
  .catch(error => {
    console.log("Bugger it didn't work");
  });
}

function connectionFault(error)
{
  hideLoader();
  $("#connectionFailedDialog").show();
  $("#errorMessage").text(error);
  console.log(":EREER");
}

function connectionEstablished()
{
  $("#loadingDialog").hide();
}

let progress = $("#loading");

$("#bleconnect").on("click", function() {
  console.log("Starting Connection");
  connectToHT();
});
