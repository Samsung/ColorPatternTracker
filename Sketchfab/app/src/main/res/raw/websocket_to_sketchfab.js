setupWebSocket();

function setupWebSocket()
{
    var url = "ws://localhost:4444/live";
    var w = new WebSocket(url);
    //var parser = new BinaryParser();

    w.onopen = function()
    {
        log("open web socket");
        w.send("accepted this Web Socket request");
    }

    w.onmessage = function(e)
    {
        log(e.data.toString());

        var xyz = e.data.toString().split(",");
        var x = parseFloat(xyz[0]) / 5.0;
        var y = parseFloat(xyz[1]) / 5.0;
        var z = parseFloat(xyz[2]) / 5.0;

        sf_api.lookat([-y, -z, x], [0, 0, 0], 0);
    }

    w.onclose = function(e)
    {
        log("closed");
    }

    w.onerror = function(e)
    {
        log("error");
    }

    window.onload = function()
    {
        document.getElementById("sendButton").onclick = function()
        {
            w.send(document.getElementById("inputMessage").value);
        }
    }
}

function log(s)
{
	var logMessage = document.getElementById("logMessage");
	var logOutput = document.getElementById("logOutput");
	logMessage.value = s;
	logOutput.innerHTML = s;
	console.log("TrackFab: " + s.toString());
}

function onDataArrived(x, y, z)
{
    console.log('data arrived' + x + " " + y + " " + z);
}

function clickFunction()
{
    console.log('clicked');

    var evt = document.createEvent("CustomEvent");
    evt.initCustomEvent("btReceiveEventType", true, true, "Any Object Here");
    evt.data = "obj data";
    //window.dispatchEvent(evt);

    document.getElementById('fullbody').dispatchEvent(evt);
}

function translateAPI()
{
    sf_api.translate(myNode, [ 1, 1, 1 ], 1.0, 'easeOutQuad',
        function(err, translateTo)
        {
            console.log('Object has been translated to', translateTo);
        }
    );
}
