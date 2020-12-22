const Discord = require('discord.js');
const axios = require('axios');
const {retrieveToken} = require('../util/interact.js');
const {responseFail} = require('./../util/responseFail.js');
const {prefix, palete} = require('../appconfig.js');

var sprintf = require('sprintf-js').sprintf;

const responseSuccess = (data, league_name, message) => {
	if (data == null)
		return responseFail(message, `You are not yet a member of this league, ${message.author}! Try joining with \`${prefix}join ${league_name}\``);
	const username = message.author.username + "*" + message.author.discriminator;

	var embed = new Discord.MessageEmbed().setColor(palete[7])
		.setTitle(message.author.username + " ⚔️ " + league_name)
		.addFields(
			{
				name: "Member Info",
				value: `🤝 Team: ${data.team || 'Not assigned'}
						🔗 Points: ${sprintf("%10.4g", data.points)}
						${data.streak_cnt > 2 ? '🔥': '♨️'}Streak: ${data.streak_cnt||0}
						🍉 Problems solved: ${data.problems_solved||0}`
			}
		);

	return embed;
}

module.exports = {
	name: 'check',
	description: "Check a user's performance during a league.",
	args: 1,
	usage: '<league_name>',
	category: 'League',
	execute(message, args) {
		const {API_URL} = process.env;
		const username = message.author.username + "*" + message.author.discriminator;

		retrieveToken().then(token => {
			axios.get(`${API_URL}/api/event/league/getMembers?league_name=${args[0]}&usernames=${username}`, {
				headers: {
					'Authentication': token
				}
			}).then(
				(res) => {
					message.channel.send(responseSuccess(res.data[0], args[0], message));
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