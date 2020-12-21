const axios = require('axios');

var token = null, expireTime = 0;

function retrieveToken() {
	const url = process.env.API_URL;
	const username = process.env.INTERACT_USERNAME;
	const password = process.env.INTERACT_PASSWORD;

	return new Promise((resolve,reject) => {

		if (expireTime <= Date.now().valueOf()) {
			axios.post(`${url}/api/auth/getToken`, {username: username, password: password}).then(function(response) {
				if (response.status == 200) {
					token = response.data;
					expireTime = Date.now().valueOf() + 60*60*12; // valid for half a day(it's in fact valid for more than this)
					resolve(token);
				} else {
					reject(`Error code: ${response.status}. Comment: ${response.data.comment}`);
				}
			}, reject);
		} else {
			resolve(token);
		}
	});
}


module.exports = {
	retrieveToken: retrieveToken
}