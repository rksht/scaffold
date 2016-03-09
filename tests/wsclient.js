var URL = "ws://mace:9090/chat";

function create_connection() {
	var conn = new WebSocket(URL);

	conn.onopen = function(e) {
		conn.send("Opened");
	};

	conn.onclose = function() {
		console.log("Connection closed");
	}

	conn.onmessage = function(m) {
		console.log("Received: " + m.data);
	}

	return conn;
}
