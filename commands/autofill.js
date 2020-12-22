const axios = require('axios');
const {retrieveToken} = require('../util/interact.js');
const {responseFail} = require('./../util/responseFail.js');

const responseSuccess = (response) => {
	return `Operation success! Server responded with: ${response}`;
}

module.exports = {
	name: 'autoFill',
	description: "Assign the user to the weakest team in the league (by gain_rate).",
	args: 1,
	usage: '<league_name>',
	guildOnly: true,
	category: 'League',
	execute(message, args) {
		const {API_URL} = process.env;

		const username = message.author.username + "*" + message.author.discriminator;

		retrieveToken().then(token => {
			axios.get(`${API_URL}/api/event/league/autofillMember?league_name=${args[0]}&username=${username}`, {
				headers: {
					'Authentication': token
				}
			}).then(
				(res) => {
					message.channel.send(responseSuccess(res.data.comment));
				},
				(err) => { 
					message.channel.send(responseFail(message, err.response.data.comment));
				}
			);
		}, (err) => { 
			message.channel.send(responseFail(message, err.response || `Seems like the server refuses to connect.`)); 
		});
	}
}