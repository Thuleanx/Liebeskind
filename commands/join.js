const axios = require('axios');
const {retrieveToken} = require('../util/interact.js');
const {responseFail} = require('./../util/responseFail.js');
const {prefix} = require('../appconfig.js');

const responseSuccess = (league_name, username) => {
	return `${username} has joined ${league_name}. Enjoy problem solving!`;
}

module.exports = {
	name: 'join',
	description: 'Have the current user join a league',
	args: 1,
	usage: '<league_name>',
	guildOnly: true,
	category: 'League',
	execute(message, args) {
		const {API_URL} = process.env;
		const username = message.author.username + "*" + message.author.discriminator;

		retrieveToken().then(token => {
			axios.get(`${API_URL}/api/event/league/addMembers?league_name=${args[0]}&members=${username}`, {
				headers: {
					'Authentication': token
				}
			}).then(
				(res) => {
					message.channel.send(responseSuccess(args[0], message.author));
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