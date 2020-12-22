const axios = require('axios');
const {retrieveToken} = require('../util/interact.js');
const {responseFail} = require('./../util/responseFail.js');
const {prefix} = require('../appconfig.js');

const responseSuccess = (league_name) => {
	return `League ${league_name} successfully created. Now anyone can join ${league_name} by \`${prefix}join ${league_name}\`.`;
}

module.exports = {
	name: 'createLeague',
	description: 'Get info of the current user',
	args: 2,
	usage: '<league_name> <team0;team1;team2;...>',
	guildOnly: true,
	adminOnly: true,
	category: 'League',
	execute(message, args) {
		const {API_URL} = process.env;

		retrieveToken().then(token => {
			axios.get(`${API_URL}/api/event/league/create?league_name=${args[0]}&teams=${args[1]}`, {
				headers: {
					'Authentication': token
				}
			}).then(
				(res) => {
					message.channel.send(responseSuccess(args[0]));
				},
				(err) => { 
					if (err.response)
						message.channel.send(responseFail(message, err.response.data.comment));
					else
						message.channel.send(responseFail(message, err));
				}
			);
		}, (err) => { 
			message.channel.send(responseFail(message, err.response || `Seems like the server refuses to connect.`)); 
		});
	}
}