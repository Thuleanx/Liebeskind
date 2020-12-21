const axios = require('axios');
const {retrieveToken} = require('../util/interact.js');
const {responseFail} = require('./../util/responseFail.js');
const {prefix} = require('../appconfig.js');

const responseSuccess = (league_name) => {
	return `${league_name} has ended. Use \`${prefix}info ${league_name}\` to see the scoreboard and official result.`;
}

module.exports = {
	name: 'endLeague',
	description: `End a league. Will update all of the user's performance, after which everything is recorded and finalized.`,
	args: 1,
	usage: '<league_name>',
	guildOnly: true,
	adminOnly: true,
	category: 'League',
	cooldown: 60*30,
	execute(message, args) {
		const {API_URL} = process.env;

		retrieveToken().then(token => {
			axios.get(`${API_URL}/api/event/league/end?league_name=${args[0]}`, {
				headers: {
					'Authentication': token
				}
			}).then(
				(res) => {
					message.channel.send(responseSuccess(args[0]));
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