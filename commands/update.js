const axios = require('axios');
const {retrieveToken} = require('../util/interact.js');
const {responseFail} = require('./../util/responseFail.js');
const {prefix} = require('../appconfig.js');

const responseSuccess = (league_name, user) => {
	return `Your performance in ${league_name} is updated, ${user}. You can now check it with \`${prefix}check ${league_name}\``;
}

module.exports = {
	name: 'update',
	description: `Update user's performance during a league`,
	args: 1,
	usage: '<league_name>',
	guildOnly: true,
	category: 'League',
	cooldown: 10,
	execute(message, args) {
		const {API_URL} = process.env;
		const username = message.author.username + "*" + message.author.discriminator;

		retrieveToken().then(token => {
			axios.get(`${API_URL}/api/event/league/updateMember?league_name=${args[0]}&username=${username}`, {
				headers: {
					'Authentication': token
				}
			}).then(
				(res) => {
					message.channel.send(responseSuccess(args[0], message.author));
				},
				(err) => { 
					console.log(err.response.data.comment);
					message.channel.send(responseFail(message, err.response.data.comment));
				}
			);
		}, (err) => { 
			message.channel.send(responseFail(message, err.response || `Seems like the server refuses to connect.`)); 
		});
	}
}